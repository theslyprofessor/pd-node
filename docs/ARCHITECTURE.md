# pd-node Architecture

**Phase 2 Implementation - How It All Works**

---

## Overview

pd-node brings npm packages and TypeScript to Pure Data by spawning Bun/Node.js as a child process and communicating via JSON over stdin/stdout.

```
┌─────────────────┐
│   Pure Data     │
│   Patch         │
└────────┬────────┘
         │
    [node script.js]
         │
┌────────▼────────┐
│  node.pd_darwin │  C++ External
│  (node.cpp)     │  • Detects runtime
└────────┬────────┘  • Spawns process
         │            • Manages IPC
    IPCBridge
         │
┌────────▼────────┐
│   fork/exec     │
│   Bun/Node.js   │  Child Process
└────────┬────────┘
         │
┌────────▼────────┐
│   wrapper.js    │  Injection Layer
│                 │  • Sets up globals
│                 │  • Handles messages
│                 │  • Provides pd-api
└────────┬────────┘
         │
┌────────▼────────┐
│  user script    │  Your Code
│  (hello.js)     │  • require('pd-api')
│                 │  • npm packages
│                 │  • async/await
└─────────────────┘
```

---

## Message Flow

### Pure Data → JavaScript

```
1. User bangs [node] object in PD
   ↓
2. node_bang() called in C++ (node.cpp)
   ↓
3. Create JSON message:
   {"type":"message","inlet":0,"selector":"bang","args":[]}
   ↓
4. IPCBridge::send_message() writes to stdin pipe
   ↓
5. wrapper.js receives on process.stdin
   ↓
6. Parses JSON, calls __pd_internal__.dispatch()
   ↓
7. Finds registered handlers from pd.on('bang', ...)
   ↓
8. Executes user's callback
```

### JavaScript → Pure Data

```
1. User calls pd.outlet(0, 42)
   ↓
2. pd-api calls __pd_internal__.outlet(0, 'float', 42)
   ↓
3. wrapper.js creates JSON:
   {"type":"outlet","outlet":0,"selector":"float","args":[42]}
   ↓
4. Writes to process.stdout
   ↓
5. IPCBridge::try_receive_message() reads from stdout pipe
   ↓
6. Parses JSON in C++
   ↓
7. Calls outlet_float(x->outlets[0], 42)
   ↓
8. Message appears in Pure Data patch
```

---

## Component Breakdown

### 1. Runtime Detector (`runtime_detector.cpp`)

**Purpose:** Find Bun or Node.js on the system

**Logic:**
```cpp
if (file.endsWith(".ts")) {
    return Runtime::BUN;  // TypeScript requires Bun
} else if (bunAvailable) {
    return Runtime::BUN;  // Prefer Bun (faster)
} else {
    return Runtime::NODE; // Fallback to Node.js
}
```

**Detection:**
- Searches PATH for `bun` and `node`
- Executes `--version` to verify
- Caches results

---

### 2. IPC Bridge (`ipc_bridge.cpp`)

**Purpose:** Spawn and communicate with child process

**Key Methods:**

```cpp
bool spawn() {
    // 1. Create pipes
    pipe(stdin_pipe_);
    pipe(stdout_pipe_);
    pipe(stderr_pipe_);
    
    // 2. Fork
    child_pid_ = fork();
    
    // 3. Child: redirect stdio and exec
    if (child_pid_ == 0) {
        dup2(stdin_pipe_[0], STDIN_FILENO);
        dup2(stdout_pipe_[1], STDOUT_FILENO);
        execl(runtime_path, runtime_path, wrapper_path, script_path, NULL);
    }
    
    // 4. Parent: close unused ends, set non-blocking
    set_nonblocking(stdout_pipe_[0]);
}

void send_message(const string& json) {
    write(stdin_pipe_[1], json + "\n");
}

bool try_receive_message(string& out) {
    // Non-blocking read
    // Returns true if complete line available
}
```

**Non-Blocking I/O:**
- PD is single-threaded
- Can't block waiting for JS
- Use fcntl() with O_NONBLOCK
- Poll in PD's event loop

---

### 3. wrapper.js (Injection Layer)

**Purpose:** Set up pd-api environment before user script loads

**Responsibilities:**

1. **Create `__pd_internal__` global:**
   ```javascript
   global.__pd_internal__ = {
       handlers: {},
       register(selector, handler) { ... },
       dispatch(msg) { ... },
       outlet(outlet, selector, ...args) { ... },
       post(message) { ... },
       error(message) { ... }
   };
   ```

2. **Override console:**
   ```javascript
   console.log = (...args) => {
       __pd_internal__.post(args.join(' '));
   };
   ```

3. **Handle stdin messages:**
   ```javascript
   process.stdin.on('data', (data) => {
       const msg = JSON.parse(line);
       if (msg.type === 'message') {
           __pd_internal__.dispatch(msg);
       }
   });
   ```

4. **Load user script:**
   ```javascript
   require(process.argv[2]);
   ```

---

### 4. pd-api Package (`pd-api/index.js`)

**Purpose:** Clean API for user scripts (like max-api)

**User-Facing API:**

```javascript
const pd = require('pd-api');

// Register handlers
pd.on('bang', () => { ... });
pd.on('float', (value) => { ... });
pd.on('list', (...values) => { ... });

// Send output
pd.outlet(0, 42);              // Float
pd.outlet(0, 'hello');         // Symbol
pd.outlet(0, [1, 2, 3]);       // List

// Console
pd.post('Debug message');
pd.error('Error message');
```

**Implementation:**
```javascript
const pd = {
    on(selector, callback) {
        __pd_internal__.register(selector, callback);
    },
    
    outlet(outlet, ...values) {
        // Auto-detect type
        if (values.length === 0) {
            __pd_internal__.outlet(outlet, 'bang');
        } else if (typeof values[0] === 'number') {
            __pd_internal__.outlet(outlet, 'float', values[0]);
        }
        // ... etc
    }
};
```

---

### 5. node.cpp (Main External)

**Purpose:** PD external entry point

**Key Functions:**

```cpp
void *node_new(t_symbol *s, int argc, t_atom *argv) {
    // 1. Get script path from argument
    string script = atom_getsymbol(&argv[0])->s_name;
    
    // 2. Detect runtime
    RuntimeDetector detector;
    Runtime runtime = detector.get_runtime_for_script(script);
    
    // 3. Create IPC bridge
    x->bridge = new IPCBridge(runtime_path, script_path);
    
    // 4. Set up message callback
    x->bridge->on_message([x](const string& json) {
        handle_json_message(x, json);
    });
    
    // 5. Spawn process
    x->bridge->spawn();
    
    // 6. Set up polling
    x->poll_clock = clock_new(x, (t_method)node_poll);
    clock_delay(x->poll_clock, 1);  // Poll every 1ms
}

void node_bang(t_node *x) {
    string msg = R"({"type":"message","inlet":0,"selector":"bang","args":[]})";
    x->bridge->send_message(msg);
}

void node_poll(t_node *x) {
    // Check for messages from JS
    string msg;
    while (x->bridge->try_receive_message(msg)) {
        handle_json_message(x, msg);
    }
    
    // Schedule next poll
    clock_delay(x->poll_clock, 1);
}
```

---

## JSON Message Protocol

### C++ → JavaScript (Incoming)

```json
{
  "type": "message",
  "inlet": 0,
  "selector": "bang|float|symbol|list",
  "args": [...]
}
```

**Examples:**
```json
{"type":"message","inlet":0,"selector":"bang","args":[]}
{"type":"message","inlet":0,"selector":"float","args":[440]}
{"type":"message","inlet":0,"selector":"symbol","args":["hello"]}
{"type":"message","inlet":0,"selector":"list","args":[1,2,3]}
```

### JavaScript → C++ (Outgoing)

```json
{
  "type": "outlet|log|error|ready",
  "outlet": 0,
  "selector": "bang|float|symbol|list",
  "args": [...],
  "message": "..."
}
```

**Examples:**
```json
{"type":"outlet","outlet":0,"selector":"float","args":[440]}
{"type":"log","message":"Hello from JavaScript"}
{"type":"error","message":"Something went wrong"}
{"type":"ready"}
```

---

## Performance Considerations

### Why Non-Blocking?

Pure Data runs on a single thread with an event loop. Blocking on I/O would freeze the entire audio graph.

### Polling Strategy

**Current:** Poll every 1ms via PD clock
- Low latency (~1ms)
- Minimal CPU impact
- Good enough for most use cases

**Alternative:** Use `select()` with timeout=0
- More efficient
- Harder to integrate with PD's clock system

### Message Buffering

- Read buffer accumulates partial messages
- Only dispatch on complete lines (newline-delimited)
- Prevents partial JSON parsing errors

---

## Error Handling

### Process Crashes

```cpp
if (!x->bridge->is_running()) {
    pd_error(x, "[node] Process crashed");
    // Auto-restart? Or require manual reload?
}
```

### JSON Parse Errors

```javascript
try {
    const msg = JSON.parse(line);
} catch (err) {
    __pd_internal__.error('Parse error: ' + err.message);
}
```

### Script Load Errors

```javascript
try {
    require(userScript);
} catch (err) {
    __pd_internal__.error('Load failed: ' + err.message);
    process.exit(1);
}
```

---

## Advantages Over pdjs

| Feature | pdjs | pd-node |
|---------|------|---------|
| **Engine** | Embedded V8 | System Bun/Node.js |
| **Size** | Large (~10MB+) | Tiny (~53KB) |
| **npm packages** | Limited | Full support |
| **TypeScript** | Manual setup | Native via Bun |
| **Updates** | Rebuild needed | System updates |
| **async/await** | Yes | Yes |
| **Debugging** | Harder | Standard Node tools |

---

## Advantages Over Max/MSP [node.script]

| Feature | Max node.script | pd-node |
|---------|-----------------|---------|
| **Download** | 2GB (bundled Node) | <1MB |
| **Runtime** | Bundled | System (user controls version) |
| **Open Source** | No | Yes (MIT) |
| **Cost** | Requires Max ($) | Free |
| **TypeScript** | Via transpiler | Native |

---

## Security Considerations

### Arbitrary Code Execution

`[node]` can run any JavaScript - this is by design but has security implications:

- ✅ **Sandboxing:** Child process can't crash PD
- ⚠️ **File access:** JS has full filesystem access
- ⚠️ **Network:** Can make HTTP requests
- ⚠️ **Exec:** Could spawn other processes

**Mitigation:**
- User must trust the scripts they load
- Similar trust model to Max/MSP
- Future: Add permission system?

---

## Future Enhancements

### Phase 3 Ideas

1. **Auto-reload on file change**
   - Watch script with fs.watch()
   - Restart process on save
   - Great for development

2. **Multiple instances**
   - Allow multiple `[node]` objects
   - Shared or isolated processes?

3. **Shared modules**
   - Cache require() across instances
   - Reduce memory usage

4. **Source maps**
   - Better TypeScript error messages
   - Line numbers match .ts files

5. **REPL mode**
   - Interactive JavaScript console
   - Send arbitrary code at runtime

---

## Development Workflow

### Building

```bash
cd ~/Code/github.com/theslyprofessor/pd-node
cd build && make -j4
```

### Installing

```bash
./install.sh  # Installs to both PD and Plugdata
```

### Testing

```bash
# 1. Restart Pure Data
# 2. Create [node examples/hello.js]
# 3. Bang it
# 4. Check console
```

### Debugging

**C++ side:**
```bash
lldb /Applications/Pd-0.54-1.app/Contents/Resources/bin/pd
```

**JavaScript side:**
```javascript
console.log('Debug:', value);  // Appears in PD console
```

---

## File Organization

```
pd-node/
├── node/
│   ├── node.cpp              ← Main external
│   ├── runtime_detector.cpp  ← Find Bun/Node
│   ├── ipc_bridge.cpp        ← Process spawning
│   └── wrapper.js            ← Injection layer
├── pd-api/
│   ├── index.js              ← User-facing API
│   └── index.d.ts            ← TypeScript defs
├── examples/
│   ├── hello.js              ← Basic example
│   └── hello.ts              ← TypeScript example
└── binaries/
    └── arm64-macos/
        ├── node.pd_darwin    ← Compiled external
        ├── wrapper.js        ← Copied here
        └── pd-api/           ← Copied here
```

---

## Key Insights

### Why wrapper.js?

Can't inject `require('pd-api')` hook directly - need to set up globals before user script runs.

### Why fork/exec instead of threads?

- Isolation: JS crash won't crash PD
- Standard: Works like any Node.js app
- Debugging: Use regular Node tools

### Why Bun preference?

- TypeScript works natively
- Faster startup (~3x)
- Modern APIs
- Node.js fallback for compatibility

### Why JSON over binary?

- Simple to debug
- Human-readable
- Good enough performance
- Easy to extend

---

**Current Status:** Phase 2 @ 50% | IPC bridge complete | Integration next

**Next:** Wire up node.cpp to actually use the bridge and parse JSON!
