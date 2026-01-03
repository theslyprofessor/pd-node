# pd-node Architecture - Deep Dive Explanation

**"How does C++ work with Pure Data's C API? How does JavaScript talk to C++?"**

Let me explain every layer from bottom to top.

---

## The Big Picture

```
┌──────────────────────────────────────────────────────┐
│  Pure Data (C code)                                  │
│  - Audio engine running in single thread             │
│  - Event loop processing messages                    │
└──────────────┬───────────────────────────────────────┘
               │
          C API calls
               │
┌──────────────▼───────────────────────────────────────┐
│  node.pd_darwin (C++ compiled to C-compatible ABI)   │
│  - Implements PD external interface                  │
│  - Spawns child processes                            │
│  - Non-blocking I/O pipes                            │
└──────────────┬───────────────────────────────────────┘
               │
        fork() + exec()
               │
┌──────────────▼───────────────────────────────────────┐
│  Bun/Node.js child process                           │
│  - Separate OS process                               │
│  - stdin/stdout connected to parent via pipes        │
└──────────────┬───────────────────────────────────────┘
               │
      JSON over pipes
               │
┌──────────────▼───────────────────────────────────────┐
│  wrapper.js (injected first)                         │
│  - Sets up global.__pd_internal__                    │
│  - Listens on stdin for JSON messages                │
│  - Provides pd-api interface                         │
└──────────────┬───────────────────────────────────────┘
               │
     require() user script
               │
┌──────────────▼───────────────────────────────────────┐
│  Your JavaScript (hello.js)                          │
│  - Uses pd-api                                       │
│  - Uses npm packages                                 │
│  - async/await, TypeScript, etc.                     │
└──────────────────────────────────────────────────────┘
```

---

## Part 1: Pure Data's C API and C++ Compatibility

### Question: "Pure Data externals are in C, not C++. How are we using C++?"

**Answer:** We use `extern "C"` to make C++ code callable from C.

#### The C++ File (node.cpp)

```cpp
// This is C++ code with classes, std::string, etc.
#include <m_pd.h>  // Pure Data's C header
#include <string>
#include <vector>

using namespace std;

// Pure C++ class
class IPCBridge {
    string command;
    int pid;
public:
    void spawn() { ... }
    void send_message(const string& msg) { ... }
};

// This struct uses C++ features
typedef struct _node {
    t_object x_obj;        // C struct from PD
    IPCBridge* bridge;     // C++ pointer!
    string script_path;    // C++ std::string!
} t_node;

// Here's the magic - extern "C" block
extern "C" {
    // This function is EXPORTED with C calling convention
    // PD can call it even though we're in C++
    void node_setup(void) {
        node_class = class_new(
            gensym("node"),
            (t_newmethod)node_new,  // Our C++ function
            (t_method)node_free,
            sizeof(t_node),
            CLASS_DEFAULT,
            A_GIMME,
            0
        );
        
        class_addbang(node_class, node_bang);
        // ...
    }
}
```

#### What `extern "C"` Does

**Without `extern "C"`:**
```cpp
void node_setup()
// Compiler mangles to: _Z10node_setupv (C++ name mangling)
// PD can't find it!
```

**With `extern "C"`:**
```cpp
extern "C" void node_setup()
// Compiler keeps name: _node_setup (C calling convention)
// PD can find and call it!
```

#### The Compilation Process

```bash
# Compile C++ to object file
g++ -c node.cpp -o node.o

# Link as shared library (.pd_darwin on macOS)
g++ -shared -o node.pd_darwin node.o

# Result: C-compatible .pd_darwin file
# PD loads it with dlopen() - doesn't know it's C++!
```

---

## Part 2: How Messages Flow (PD ↔ C++ ↔ JavaScript)

### Flow 1: User Bangs Object in Pure Data

#### Step 1: Pure Data's C Code
```c
// Inside PD's source (s_inter.c)
void pd_bang(t_pd *x) {
    (*x->c_bangmethod)(x);  // Calls our node_bang()
}
```

#### Step 2: Our C++ Handler
```cpp
// node.cpp
static void node_bang(t_node *x) {
    // Create JSON message
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", "bang"},
        {"args", json::array()}
    };
    
    // Convert to string
    string json_str = msg.dump();  // {"type":"message"...}
    
    // Send to JavaScript process via pipe
    x->bridge->send_message(json_str);
}
```

#### Step 3: IPC Bridge Sends to Child Process
```cpp
// ipc_bridge.cpp
void IPCBridge::send_message(const string& msg) {
    // Write to stdin pipe (connected to child process)
    string line = msg + "\n";
    ssize_t written = write(stdin_pipe_[1], line.c_str(), line.size());
    
    if (written < 0) {
        perror("write failed");
    }
}
```

**What's happening:**
- `stdin_pipe_[1]` is the **write end** of a Unix pipe
- Writing to it sends data to child process's `stdin`
- The `\n` is crucial - newline-delimited JSON

#### Step 4: JavaScript Receives Message
```javascript
// wrapper.js
process.stdin.on('data', (chunk) => {
    buffer += chunk.toString();
    
    // Split on newlines
    const lines = buffer.split('\n');
    buffer = lines.pop();  // Keep incomplete line
    
    for (const line of lines) {
        if (!line.trim()) continue;
        
        try {
            const msg = JSON.parse(line);
            handleMessage(msg);
        } catch (err) {
            console.error('Parse error:', err);
        }
    }
});

function handleMessage(msg) {
    if (msg.type === 'message') {
        // Find handler registered by user
        const handlers = __pd_internal__.handlers[msg.selector] || [];
        
        for (const handler of handlers) {
            handler(msg.inlet, ...msg.args);
        }
    }
}
```

#### Step 5: User's Handler Executes
```javascript
// hello.js (user's script)
const pd = require('pd-api');

pd.on('bang', () => {
    // THIS FUNCTION RUNS NOW!
    console.log('Got bang!');
    pd.outlet(0, 'Hello!');
});
```

---

### Flow 2: JavaScript Sends Message Back to Pure Data

#### Step 1: User Calls pd.outlet()
```javascript
// hello.js
pd.outlet(0, 'Hello from JavaScript!');
```

#### Step 2: pd-api Creates Message
```javascript
// pd-api/index.js
function outlet(outlet_num, ...values) {
    let selector, args;
    
    if (values.length === 0) {
        selector = 'bang';
        args = [];
    } else if (typeof values[0] === 'string') {
        selector = 'symbol';
        args = [values[0]];
    } else if (typeof values[0] === 'number') {
        selector = 'float';
        args = [values[0]];
    }
    
    __pd_internal__.outlet(outlet_num, selector, args);
}
```

#### Step 3: wrapper.js Sends JSON to stdout
```javascript
// wrapper.js
__pd_internal__.outlet = function(outlet, selector, args) {
    const msg = {
        type: 'outlet',
        outlet: outlet,
        selector: selector,
        args: args
    };
    
    // Write to stdout (connected to parent's pipe)
    process.stdout.write(JSON.stringify(msg) + '\n');
};
```

#### Step 4: C++ Reads from Pipe
```cpp
// ipc_bridge.cpp
bool IPCBridge::try_receive_message(string& out) {
    char buffer[4096];
    
    // Non-blocking read from stdout pipe
    ssize_t n = read(stdout_pipe_[0], buffer, sizeof(buffer));
    
    if (n > 0) {
        read_buffer_ += string(buffer, n);
        
        // Look for complete line
        size_t pos = read_buffer_.find('\n');
        if (pos != string::npos) {
            out = read_buffer_.substr(0, pos);
            read_buffer_.erase(0, pos + 1);
            return true;  // Got complete message!
        }
    }
    
    return false;  // No complete message yet
}
```

#### Step 5: C++ Parses JSON and Calls PD
```cpp
// node.cpp
static void handle_json_message(t_node *x, const string& json_str) {
    try {
        json msg = json::parse(json_str);
        
        if (msg["type"] == "outlet") {
            int outlet = msg["outlet"];
            string selector = msg["selector"];
            auto args = msg["args"];
            
            if (selector == "symbol") {
                t_symbol *sym = gensym(args[0].get<string>().c_str());
                outlet_symbol(x->outlet, sym);
            } else if (selector == "float") {
                outlet_float(x->outlet, args[0].get<double>());
            }
            // ...
        }
    } catch (exception& e) {
        pd_error(x, "[node] JSON parse error: %s", e.what());
    }
}
```

#### Step 6: Pure Data Receives Message
```c
// Pure Data's code
void outlet_symbol(t_outlet *x, t_symbol *s) {
    // Sends symbol to connected objects in patch
    pd_typedmess(&x->dest->x_pd, s, 0, NULL);
}
```

---

## Part 3: Process Spawning (fork/exec)

### Question: "How do we spawn Bun/Node.js?"

#### The Code
```cpp
// ipc_bridge.cpp
bool IPCBridge::spawn() {
    // 1. Create three pipes (stdin, stdout, stderr)
    if (pipe(stdin_pipe_) < 0 || 
        pipe(stdout_pipe_) < 0 || 
        pipe(stderr_pipe_) < 0) {
        return false;
    }
    
    // 2. Fork creates a copy of this process
    child_pid_ = fork();
    
    if (child_pid_ == 0) {
        // ===== CHILD PROCESS =====
        
        // 3. Redirect stdin/stdout/stderr to pipes
        dup2(stdin_pipe_[0], STDIN_FILENO);    // Read from pipe
        dup2(stdout_pipe_[1], STDOUT_FILENO);  // Write to pipe
        dup2(stderr_pipe_[1], STDERR_FILENO);  // Errors to pipe
        
        // 4. Close unused pipe ends
        close(stdin_pipe_[1]);   // Won't write to own stdin
        close(stdout_pipe_[0]);  // Won't read from own stdout
        
        // 5. Replace process with Bun/Node
        const char *args[] = {
            runtime_path_.c_str(),    // "/Users/.../bun"
            wrapper_path_.c_str(),    // "wrapper.js"
            script_path_.c_str(),     // "hello.js"
            NULL
        };
        
        execv(runtime_path_.c_str(), (char *const *)args);
        
        // If exec fails, we get here
        perror("execv failed");
        exit(1);
        
    } else {
        // ===== PARENT PROCESS (our C++ code) =====
        
        // 6. Close unused pipe ends
        close(stdin_pipe_[0]);   // Won't read from stdin
        close(stdout_pipe_[1]);  // Won't write to stdout
        close(stderr_pipe_[1]);
        
        // 7. Make reading non-blocking
        int flags = fcntl(stdout_pipe_[0], F_GETFL, 0);
        fcntl(stdout_pipe_[0], F_SETFL, flags | O_NONBLOCK);
        
        return true;
    }
}
```

#### What Happens in Memory

**Before fork():**
```
┌─────────────────────┐
│  Pure Data Process  │
│  PID: 12345         │
│                     │
│  [node.pd_darwin]   │
│  loaded in memory   │
└─────────────────────┘
```

**After fork():**
```
┌─────────────────────┐         ┌─────────────────────┐
│  Pure Data Process  │         │  Child Process      │
│  PID: 12345         │         │  PID: 12346         │
│                     │         │                     │
│  [node.pd_darwin]   │ ◄─pipe─►│  (exact copy)       │
│  Parent code runs   │         │  Child code runs    │
└─────────────────────┘         └─────────────────────┘
```

**After execv():**
```
┌─────────────────────┐         ┌─────────────────────┐
│  Pure Data Process  │         │  Bun Process        │
│  PID: 12345         │         │  PID: 12346         │
│                     │         │                     │
│  [node.pd_darwin]   │ ◄─pipe─►│  Running wrapper.js │
│  Writing JSON       │         │  Reading JSON       │
└─────────────────────┘         └─────────────────────┘
```

---

## Part 4: Non-Blocking I/O

### Question: "Why non-blocking? What's the polling?"

#### The Problem
Pure Data runs audio in **one thread**. If we do:

```cpp
// BAD - This blocks!
char buffer[1024];
read(pipe, buffer, 1024);  // Waits forever if no data
// Audio stops! Disaster!
```

#### The Solution: Non-Blocking I/O + Polling

```cpp
// 1. Make pipe non-blocking
fcntl(stdout_pipe_[0], F_SETFL, O_NONBLOCK);

// 2. Try to read without blocking
ssize_t n = read(stdout_pipe_[0], buffer, 1024);

if (n > 0) {
    // Got data!
} else if (n == 0) {
    // EOF - process died
} else if (errno == EAGAIN) {
    // No data available - THAT'S OK!
    // We'll try again later
}
```

#### Polling with PD's Clock System

```cpp
// node.cpp
static void *node_new(...) {
    // Create a clock that calls node_poll() every 1ms
    x->poll_clock = clock_new(x, (t_method)node_poll);
    clock_delay(x->poll_clock, 1);  // 1ms delay
}

static void node_poll(t_node *x) {
    // Try to read messages (non-blocking)
    string msg;
    while (x->bridge->try_receive_message(msg)) {
        handle_json_message(x, msg);
    }
    
    // Schedule next poll
    clock_delay(x->poll_clock, 1);  // Check again in 1ms
}
```

**Timeline:**
```
t=0ms:   node_poll() runs → checks pipe → no data → schedules t=1ms
t=1ms:   node_poll() runs → checks pipe → no data → schedules t=2ms
t=2ms:   node_poll() runs → checks pipe → GOT DATA! → parse → schedules t=3ms
t=3ms:   node_poll() runs → checks pipe → no data → schedules t=4ms
...
```

This way audio keeps running smoothly!

---

## Part 5: The Complete Message Journey

### Example: User bangs [node hello.js], JS sends back "Hello!"

```
1. User clicks bang in Pure Data patch
   ↓
2. PD calls class_addbang handler → node_bang(x)
   ↓
3. node_bang() creates JSON: {"type":"message","selector":"bang"...}
   ↓
4. write(stdin_pipe, json) → bytes go to Bun's stdin
   ↓
5. wrapper.js receives data on process.stdin
   ↓
6. JSON.parse() → finds handler for 'bang'
   ↓
7. User's pd.on('bang', () => {...}) executes
   ↓
8. User calls pd.outlet(0, 'Hello!')
   ↓
9. pd-api creates JSON: {"type":"outlet","selector":"symbol","args":["Hello!"]}
   ↓
10. process.stdout.write(json) → bytes go to C++ pipe
    ↓
11. node_poll() runs (every 1ms)
    ↓
12. read(stdout_pipe) gets bytes (non-blocking)
    ↓
13. Accumulate until \n found → complete JSON line
    ↓
14. JSON::parse() in C++
    ↓
15. outlet_symbol(x->outlet, "Hello!")
    ↓
16. PD sends "Hello!" to connected objects
    ↓
17. [print] object shows "Hello!" in console
```

**Total latency:** ~1-2ms (polling interval)

---

## Part 6: Why This Design?

### Alternative 1: Embedded JavaScript Engine (like pdjs)

**pdjs approach:**
- Embed V8 engine in C++
- Call JS functions directly from C++

**Pros:**
- Lower latency (no IPC)
- Simpler message passing

**Cons:**
- Large binary size (~10MB+ with V8)
- Must compile V8 for each platform
- npm packages need special handling
- Updates require recompiling external

### Alternative 2: Shared Memory

**Shared memory approach:**
- Parent and child share memory region
- No pipe I/O needed

**Pros:**
- Extremely fast

**Cons:**
- Complex synchronization
- Crash in JS can corrupt PD memory
- Platform-specific code

### Our Choice: Pipes + Child Process

**Pros:**
- ✅ Tiny binary (53KB)
- ✅ Complete isolation (JS crash = external dies, PD lives)
- ✅ Standard Node.js environment
- ✅ User controls runtime version
- ✅ Full npm ecosystem works
- ✅ Easy debugging (attach to child PID)

**Cons:**
- ❌ ~1-2ms latency (acceptable for most uses)
- ❌ JSON parsing overhead (minimal)

---

## Part 7: Key Files Explained

### node.cpp (363 lines)
- **Purpose:** PD external interface
- **Exports:** `node_setup()` via `extern "C"`
- **Creates:** Outlets, clocks, IPCBridge
- **Handles:** PD messages (bang, float, symbol, list)
- **Polls:** Child process for responses

### ipc_bridge.cpp
- **Purpose:** Process management
- **Methods:**
  - `spawn()` - fork/exec child
  - `send_message()` - write JSON to stdin
  - `try_receive_message()` - read JSON from stdout (non-blocking)
- **State:** Pipe file descriptors, child PID, read buffer

### wrapper.js (121 lines)
- **Purpose:** Bootstrap environment
- **Sets up:** global.__pd_internal__
- **Listens:** process.stdin for JSON
- **Overrides:** console.log/error
- **Loads:** User's script with require()

### pd-api/index.js
- **Purpose:** Clean user-facing API
- **Exports:** pd.on(), pd.outlet(), pd.post()
- **Wraps:** __pd_internal__ calls
- **Matches:** Max/MSP's max-api design

---

## Summary

**Q: How does C++ work with PD's C API?**  
**A:** `extern "C"` makes C++ functions callable from C. PD loads the .pd_darwin file and calls `node_setup()`.

**Q: How does JS talk to C++?**  
**A:** Unix pipes! JSON messages flow over stdin/stdout between parent (C++) and child (Bun/Node).

**Q: Why not embed a JS engine?**  
**A:** Smaller binary, better npm support, user-controlled runtime, complete isolation.

**Q: How does it not block audio?**  
**A:** Non-blocking I/O + 1ms polling loop using PD's clock system.

**Q: Is this production-ready?**  
**A:** Phase 2 is complete! IPC works, examples run. Phase 3 would add auto-reload, better error handling, etc.

---

**The beauty of this design:** It's simple, robust, and treats JavaScript as a first-class citizen in the Pure Data ecosystem, just like Max/MSP but open-source and tiny!
