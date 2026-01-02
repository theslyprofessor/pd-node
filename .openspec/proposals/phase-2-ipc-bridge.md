# Phase 2: IPC Bridge Implementation

**Status:** Ready to start
**Estimated time:** 1-2 weeks
**Dependencies:** ✅ Runtime detection complete, ✅ Build system working

---

## Goal

Create bidirectional communication between C++ PD external and Bun/Node.js child process.

```
PD Patch
  ↓
[node hello.js] ← C++ external
  ↓
stdin/stdout JSON
  ↓
Bun/Node.js process ← Running hello.js
  ↓
pd-api calls
  ↓
JSON messages back
  ↓
PD outlets
```

---

## Architecture

### Process Model

```cpp
// node/ipc_bridge.h
class IPCBridge {
public:
    IPCBridge(Runtime runtime, const std::string& script_path);
    ~IPCBridge();
    
    // Process management
    bool spawn_process();
    void terminate_process();
    bool is_running() const;
    
    // Communication
    void send_message(const std::string& json);
    std::string receive_message();  // Non-blocking
    
    // Event handling
    void on_stdout(std::function<void(const std::string&)> callback);
    void on_stderr(std::function<void(const std::string&)> callback);
    void on_exit(std::function<void(int)> callback);
    
private:
    pid_t child_pid_;
    int stdin_pipe_[2];
    int stdout_pipe_[2];
    int stderr_pipe_[2];
};
```

### Message Protocol (JSON)

**C++ → JavaScript:**
```json
{
  "type": "message",
  "inlet": 0,
  "selector": "bang|float|symbol|list",
  "args": [...]
}
```

**JavaScript → C++:**
```json
{
  "type": "outlet",
  "outlet": 0,
  "selector": "bang|float|symbol|list",
  "args": [...]
}
```

**Special messages:**
```json
{ "type": "ready" }           // Script loaded successfully
{ "type": "error", "message": "..." }  // Runtime error
{ "type": "log", "message": "..." }    // console.log()
```

---

## Implementation Tasks

### Task 1: Process Spawning

**File:** `node/ipc_bridge.cpp`

```cpp
bool IPCBridge::spawn_process() {
    // 1. Create pipes for stdin/stdout/stderr
    pipe(stdin_pipe_);
    pipe(stdout_pipe_);
    pipe(stderr_pipe_);
    
    // 2. Fork child process
    child_pid_ = fork();
    
    if (child_pid_ == 0) {
        // Child process
        
        // 3. Redirect stdio to pipes
        dup2(stdin_pipe_[0], STDIN_FILENO);
        dup2(stdout_pipe_[1], STDOUT_FILENO);
        dup2(stderr_pipe_[1], STDERR_FILENO);
        
        // 4. Close unused pipe ends
        close(stdin_pipe_[1]);
        close(stdout_pipe_[0]);
        close(stderr_pipe_[0]);
        
        // 5. Execute runtime with script
        execl(runtime_path_.c_str(), 
              runtime_path_.c_str(),
              wrapper_script_.c_str(),  // Our injection wrapper
              script_path_.c_str(),
              nullptr);
        
        // If we get here, exec failed
        exit(1);
    }
    
    // Parent process
    close(stdin_pipe_[0]);
    close(stdout_pipe_[1]);
    close(stderr_pipe_[1]);
    
    return child_pid_ > 0;
}
```

**Key decisions:**
- ✅ Use `fork()` + `exec()` (POSIX standard, works on macOS/Linux)
- ✅ Windows: Use `CreateProcess()` with separate implementation
- ✅ Set pipes to non-blocking mode for PD's event loop

### Task 2: Wrapper Script (Injection)

**File:** `node/wrapper.js` (embedded in C++ as string literal)

```javascript
// This script runs BEFORE user's script
// It sets up pd-api globals and message handling

const __pd_internal__ = {
    outlet: (outlet, selector, ...args) => {
        const msg = { 
            type: 'outlet', 
            outlet, 
            selector, 
            args 
        };
        process.stdout.write(JSON.stringify(msg) + '\n');
    },
    
    post: (message) => {
        const msg = { type: 'log', message };
        process.stdout.write(JSON.stringify(msg) + '\n');
    },
    
    error: (message) => {
        const msg = { type: 'error', message };
        process.stdout.write(JSON.stringify(msg) + '\n');
    }
};

// Replace console.log to route through PD
console.log = (...args) => {
    __pd_internal__.post(args.join(' '));
};

console.error = (...args) => {
    __pd_internal__.error(args.join(' '));
};

// Setup pd-api require() hook
const Module = require('module');
const originalResolve = Module._resolveFilename;

Module._resolveFilename = function(request, parent, isMain) {
    if (request === 'pd-api') {
        // Return our embedded pd-api
        return __dirname + '/pd-api/index.js';
    }
    return originalResolve(request, parent, isMain);
};

// Message handler from PD
process.stdin.on('data', (data) => {
    const lines = data.toString().split('\n');
    for (const line of lines) {
        if (!line.trim()) continue;
        
        try {
            const msg = JSON.parse(line);
            
            if (msg.type === 'message') {
                // Dispatch to user's registered handlers
                // (handled by pd-api internals)
                __pd_internal__.dispatch(msg);
            }
        } catch (err) {
            __pd_internal__.error('Parse error: ' + err.message);
        }
    }
});

// Signal ready
process.stdout.write(JSON.stringify({ type: 'ready' }) + '\n');

// Now load user's script
require(process.argv[2]);
```

**Embed in C++:**
```cpp
// node/wrapper_script.h
namespace pdnode {
    const char* WRAPPER_SCRIPT = R"(
        // ... entire wrapper.js contents ...
    )";
}
```

### Task 3: Update node.cpp to Use IPC

**File:** `node/node.cpp`

```cpp
void *node_new(t_symbol *s, int argc, t_atom *argv) {
    t_node *x = (t_node *)pd_new(node_class);
    
    // 1. Get script path from first argument
    if (argc < 1 || argv[0].a_type != A_SYMBOL) {
        pd_error(x, "[node] requires script path as argument");
        return nullptr;
    }
    
    std::string script_path = atom_getsymbol(&argv[0])->s_name;
    
    // 2. Detect runtime
    RuntimeDetector detector;
    Runtime runtime = detector.get_runtime_for_script(script_path);
    
    if (runtime == Runtime::NONE) {
        pd_error(x, "%s", detector.get_error_message(script_path).c_str());
        return nullptr;
    }
    
    // 3. Create IPC bridge
    x->bridge = new IPCBridge(runtime, script_path);
    
    // 4. Set up callbacks
    x->bridge->on_stdout([x](const std::string& line) {
        try {
            auto msg = json::parse(line);  // Use nlohmann/json or similar
            
            if (msg["type"] == "outlet") {
                // Send to PD outlet
                int outlet_num = msg["outlet"];
                std::string selector = msg["selector"];
                
                if (selector == "bang") {
                    outlet_bang(x->outlets[outlet_num]);
                } else if (selector == "float") {
                    outlet_float(x->outlets[outlet_num], msg["args"][0]);
                }
                // ... etc for symbol, list
            }
            else if (msg["type"] == "log") {
                pd_post("[node] %s", msg["message"].c_str());
            }
            else if (msg["type"] == "error") {
                pd_error(x, "[node] %s", msg["message"].c_str());
            }
        } catch (...) {
            pd_error(x, "[node] Invalid JSON from script");
        }
    });
    
    // 5. Spawn the process
    if (!x->bridge->spawn_process()) {
        pd_error(x, "[node] Failed to spawn %s", 
                 detector.get_runtime_name(runtime).c_str());
        delete x->bridge;
        return nullptr;
    }
    
    return x;
}

void node_bang(t_node *x) {
    std::string msg = R"({"type":"message","inlet":0,"selector":"bang","args":[]})";
    x->bridge->send_message(msg);
}

void node_float(t_node *x, t_float f) {
    char buf[256];
    snprintf(buf, sizeof(buf), 
             R"({"type":"message","inlet":0,"selector":"float","args":[%f]})", f);
    x->bridge->send_message(buf);
}

// ... etc for symbol, list, anything
```

### Task 4: Non-Blocking I/O

**Challenge:** PD is single-threaded. We can't block waiting for child process.

**Solution:** Use PD's clock system for polling.

```cpp
// In node_new():
x->poll_clock = clock_new(x, (t_method)node_poll_stdout);
clock_delay(x->poll_clock, 0);  // Poll immediately

// Polling function (called by PD clock):
void node_poll_stdout(t_node *x) {
    // Non-blocking read from stdout pipe
    std::string line;
    if (x->bridge->try_receive_message(line)) {
        // Process message (call on_stdout callback)
    }
    
    // Schedule next poll (e.g., every 1ms)
    clock_delay(x->poll_clock, 1);
}
```

**Alternative:** Use `select()` or `poll()` with timeout=0 in PD's idle callback.

### Task 5: pd-api Integration

**Update:** `pd-api/index.js`

```javascript
// Global injected by wrapper.js
const internal = global.__pd_internal__;

const handlers = {
    bang: [],
    float: [],
    symbol: [],
    list: [],
    // ...
};

// Dispatch from wrapper
internal.dispatch = (msg) => {
    const { selector, args } = msg;
    const handlerList = handlers[selector] || [];
    
    for (const handler of handlerList) {
        handler(...args);
    }
};

// Public API
exports.on = (selector, handler) => {
    if (!handlers[selector]) {
        handlers[selector] = [];
    }
    handlers[selector].push(handler);
};

exports.outlet = (outlet, ...args) => {
    let selector = 'list';
    if (args.length === 1) {
        if (typeof args[0] === 'number') selector = 'float';
        if (typeof args[0] === 'string') selector = 'symbol';
    }
    if (args.length === 0) selector = 'bang';
    
    internal.outlet(outlet, selector, ...args);
};

exports.post = internal.post;
exports.error = internal.error;
```

---

## Testing Strategy

### Test 1: Process Spawning
```bash
# Create minimal test
echo 'console.log("hello from node")' > test.js

# In PD:
[node test.js]
# Should see: "hello from node" in PD console
```

### Test 2: Message Passing
```javascript
// test-bang.js
const pd = require('pd-api');
pd.on('bang', () => {
    pd.outlet(0, 'received bang!');
});
```

```
# In PD:
[bang]
  |
[node test-bang.js]
  |
[print]
# Should print: "received bang!"
```

### Test 3: npm Packages
```bash
cd ~/.pd-node-scripts/
npm install lodash
```

```javascript
// test-lodash.js
const _ = require('lodash');
const pd = require('pd-api');

pd.on('list', (...args) => {
    const sorted = _.sortBy(args);
    pd.outlet(0, 'sorted:', ...sorted);
});
```

---

## Files to Create/Modify

### New Files:
- `node/ipc_bridge.h` - IPC bridge header
- `node/ipc_bridge.cpp` - Process spawning and communication
- `node/wrapper_script.h` - Embedded JavaScript wrapper
- `node/json_parser.cpp` - Lightweight JSON parsing (or use nlohmann/json)

### Modified Files:
- `node/node.cpp` - Use IPCBridge instead of stub
- `pd-api/index.js` - Add internal dispatch mechanism
- `CMakeLists.txt` - Link against any JSON library

### Dependencies:
- JSON library: Use [nlohmann/json](https://github.com/nlohmann/json) (header-only)
- Or write minimal JSON parser for our specific message format

---

## Success Criteria

✅ Phase 2 is complete when:
1. `[node hello.js]` spawns Bun/Node.js process
2. `console.log()` appears in PD console
3. Banging `[node script.js]` triggers JavaScript handler
4. JavaScript can call `pd.outlet()` and data appears in PD
5. `require('lodash')` works after `npm install lodash`
6. Process terminates cleanly when PD object is deleted

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Process zombies if PD crashes | Add signal handlers for cleanup |
| Blocking I/O stalls PD | Use non-blocking pipes + polling |
| JSON parsing overhead | Keep messages minimal, optimize parser |
| Child process stderr floods console | Rate-limit error messages |

---

## Next Phase Preview

**Phase 3: Advanced Features**
- Auto-reload on script changes (file watching)
- Multiple script instances
- Shared modules between instances
- TypeScript source maps for error reporting
- Performance profiling

