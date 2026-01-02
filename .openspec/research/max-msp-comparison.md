---
created: 2026-01-02T03:35:00-0800
updated: 2026-01-02T03:35:00-0800
author: Nakul Tiruviluamala (with Claude)
status: research
---

# Max/MSP JavaScript Architecture vs pdjs - Deep Comparison

## Executive Summary

**Goal:** Bring pdjs to parity with (and exceed) Max/MSP's Node.js capabilities by implementing modern npm package support while maintaining compatibility with Pure Data's architecture.

**Key Finding:** Max/MSP has **TWO distinct JavaScript objects** with different capabilities:
1. `[js]` - Embedded SpiderMonkey (2011), tight Max integration, no npm
2. `[node.script]` - Full Node.js runtime (v20.x), complete npm ecosystem, async process

**pdjs Current State:** Hybrid approach - modern V8 engine but basic require() similar to `[js]`

## Max/MSP's Two JavaScript Abstractions

### 1. `[js]` Object - Embedded JavaScript

**Engine:** Mozilla SpiderMonkey (Firefox 4.0 era, circa 2011)

**Capabilities:**
- ✅ Direct Max API access (Buffer, Dict, File, Patcher, Live API)
- ✅ Tight integration with Max scheduler (low/high priority threads)
- ✅ `require()` and `include()` for local files
- ✅ UI scripting (jsui for custom drawing)
- ❌ No npm packages
- ❌ No modern ES6+ features (no `let`, `const`, arrow functions, etc.)
- ❌ No async/await
- ❌ Limited to files in Max search path

**Use Cases:**
- UI work and patcher scripting
- Tight timing requirements (scheduler integration)
- Direct manipulation of Max objects (Buffer, Dict, etc.)

### 2. `[node.script]` Object - Full Node.js Runtime

**Engine:** Node.js v20.x (latest LTS bundled with Max)

**Capabilities:**
- ✅ **Full npm ecosystem** - `npm install` any package
- ✅ Complete Node.js APIs (fs, path, http, child_process, etc.)
- ✅ Modern JavaScript (ES2023+)
- ✅ Async/await, Promises
- ✅ `package.json` support
- ✅ `node_modules` resolution
- ✅ Max API via `max-api` npm package
- ✅ Separate process (survives Max crashes better)
- ✅ Remote debugger support
- ⚠️ Async-only communication (everything is message-passing)
- ⚠️ Less tight scheduler integration
- ❌ No direct Buffer/Dict manipulation (must use named dicts)

**Use Cases:**
- Web servers, APIs, network communication
- File system operations
- Database integration
- npm package usage (AI, data processing, etc.)
- Command-line tool integration

**Key Architecture:** `node.script` spawns a **child process** running Node.js alongside Max. Communication happens via:
- Max → Node: Messages sent to object
- Node → Max: `max-api` module's `outlet()` function

## pdjs Current Architecture

### What pdjs HAS (Similar to `[js]`)

**Engine:** Google V8 (latest, same as Node.js uses)

**Current Implementation:**
```cpp
// Line 650-677: js_require() implementation
static void js_require(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // 1. Get script name from argument
    auto script_name = js_object_to_string(isolate, args[0]);
    
    // 2. Create exports object: { exports: {} }
    v8::Local<v8::Object> o = v8::Object::New(isolate);
    auto exportsKey = v8::String::NewFromUtf8Literal(isolate, "exports");
    o->Set(context, exportsKey, v8::Object::New(isolate));
    
    // 3. Load script with exports object as context
    js_load(x, script_name.c_str(), false, &o);
    
    // 4. Return exports object
    o->Get(context, exportsKey).ToLocal(&exports);
    args.GetReturnValue().Set(exports);
}
```

**File Resolution (Line 605-627):**
```cpp
static js_file js_getfile(const t_js *x, const char* script_name) {
    const t_symbol* canvas_dir = canvas_getdir(x->canvas);
    char dirresult[MAXPDSTRING];
    char* nameresult;
    
    // Uses PD's open_via_path() - searches PD path only
    int fd = open_via_path(canvas_dir->s_name, script_name, "", 
                           dirresult, &nameresult, sizeof(dirresult), 1);
    
    if (fd < 0) {
        pd_error(&x->x_obj, "Script file '%s' not found.", script_name);
        return result;
    }
    
    sys_close(fd);
    result.path = string(dirresult) + "/" + nameresult;
    return result;
}
```

**What This Means:**
- ✅ Modern V8 engine (ES2023+ support)
- ✅ Basic CommonJS pattern (exports object)
- ✅ File-based module loading
- ❌ **No node_modules resolution**
- ❌ **No package.json support**
- ❌ **No npm install capability**
- ❌ No Node.js core modules (fs, path, etc.)

### Comparison Matrix

| Feature | `[js]` (Max) | `[node.script]` (Max) | pdjs (Current) | pdjs (Proposed) |
|---------|--------------|---------------------|----------------|-----------------|
| **Engine** | SpiderMonkey 2011 | Node.js v20 | V8 latest | V8 latest |
| **ES6+ Support** | ❌ | ✅ | ✅ | ✅ |
| **npm packages** | ❌ | ✅ | ❌ | ✅ |
| **node_modules resolution** | ❌ | ✅ | ❌ | ✅ |
| **package.json** | ❌ | ✅ | ❌ | ✅ |
| **require() local files** | ✅ | ✅ | ✅ | ✅ |
| **Node.js core modules** | ❌ | ✅ | ❌ | 🟡 Polyfills |
| **Async/await** | ❌ | ✅ | ✅ (engine) | ✅ (integrated) |
| **Direct API access** | ✅ | ❌ (via max-api) | ✅ | ✅ |
| **Separate process** | ❌ | ✅ | ❌ | ❌ |
| **Scheduler integration** | ✅ | ⚠️ | ✅ | ✅ |
| **Performance** | Medium | Medium (IPC) | High | High |

## Proposed pdjs Architecture

### Vision: Best of Both Worlds

**Combine:**
- V8's modern engine + performance (like `[node.script]`)
- npm package ecosystem (like `[node.script]`)
- Tight PD integration (like `[js]`)
- Single process (like `[js]`)

### Strategy: Create TWO Pure Data Objects

#### Option A: Enhance pdjs + Create pd-node

```
[js]  (pdjs - current)               [node]  (new object)
├─ V8 engine embedded                ├─ V8 engine embedded
├─ Tight PD integration              ├─ npm package support
├─ Synchronous execution             ├─ node_modules resolution
├─ Direct outlet() calls             ├─ Async event loop
└─ Max [js] compatibility            ├─ package.json support
                                     ├─ Core module polyfills
                                     └─ Max [node.script] compatibility
```

#### Option B: Unified pdjs-next

Single object with dual modes:
```javascript
// Mode 1: Classic (current behavior)
const foo = require("./local-file.js");

// Mode 2: Node mode (auto-detected by package.json presence)
const _ = require("lodash");  // Searches node_modules
const pd = require("pd-api");  // Our equivalent of max-api
```

**Recommendation:** **Option A** - Separate objects maintain clearer semantics and prevent confusion

## Technical Implementation Plan

### Phase 1: node_modules Resolution (HIGH IMPACT)

**Goal:** Make `require("package-name")` work for npm packages

**Implementation in new `[node]` object:**

```cpp
// Enhanced file resolution with node_modules traversal
static js_file js_resolve_module(const t_js *x, const char* module_name) {
    js_file result;
    
    // 1. Check if it's a relative/absolute path
    if (module_name[0] == '.' || module_name[0] == '/') {
        return js_getfile(x, module_name);  // Use existing logic
    }
    
    // 2. Check if it's a core module polyfill
    if (is_core_module(module_name)) {
        return get_core_module_path(module_name);
    }
    
    // 3. Search node_modules hierarchy (Node.js algorithm)
    string current_dir = x->dir;
    
    while (true) {
        // Try: current_dir/node_modules/module_name/package.json
        string pkg_path = current_dir + "/node_modules/" + module_name + "/package.json";
        
        if (file_exists(pkg_path)) {
            // Parse package.json for "main" field
            string main_file = parse_package_main(pkg_path);
            result.path = current_dir + "/node_modules/" + module_name + "/" + main_file;
            result.dir = current_dir + "/node_modules/" + module_name;
            return result;
        }
        
        // Try: current_dir/node_modules/module_name/index.js
        string index_path = current_dir + "/node_modules/" + module_name + "/index.js";
        if (file_exists(index_path)) {
            result.path = index_path;
            result.dir = current_dir + "/node_modules/" + module_name;
            return result;
        }
        
        // Move up directory tree
        string parent = get_parent_dir(current_dir);
        if (parent == current_dir) break;  // Hit root
        current_dir = parent;
    }
    
    pd_error(&x->x_obj, "Cannot find module '%s'", module_name);
    return result;
}
```

**Benefits:**
- ✅ Works with any npm package after `npm install`
- ✅ Standard Node.js module resolution algorithm
- ✅ No changes to existing pdjs behavior

### Phase 2: Core Module Polyfills

**Bundle browser-compatible Node.js core modules:**

```cpp
// Embed polyfills in binary or ship with external
static const char* core_modules[] = {
    "path",        // path-browserify
    "events",      // events npm package
    "util",        // util npm package
    "buffer",      // buffer npm package
    "stream",      // readable-stream
    "process",     // process npm package
    NULL
};

static string get_core_module_path(const char* name) {
    // Return path to embedded polyfill
    return string(PD_EXTERNAL_DIR) + "/pdjs/polyfills/" + name + ".js";
}
```

### Phase 3: Async Integration with PD Event Loop

**Challenge:** V8 microtasks (Promises) need event loop integration

**Solution:** Hook into PD's clock system

```cpp
// Global microtask pump scheduled via PD clock
static t_clock* js_microtask_clock;

static void js_pump_microtasks(void* dummy) {
    v8::Isolate::Scope isolate_scope(js_isolate);
    
    // Pump V8 microtask queue
    while (v8::platform::PumpMessageLoop(js_platform.get(), js_isolate)) {
        // Continue until queue empty
    }
    
    // Reschedule for next tick
    clock_delay(js_microtask_clock, 0);  // Next PD tick
}

// In js_setup()
js_microtask_clock = clock_new(NULL, (t_method)js_pump_microtasks);
clock_delay(js_microtask_clock, 0);
```

**Enables:**
- ✅ `async`/`await` integration
- ✅ Promise chains
- ✅ Timer functions (`setTimeout`, `setInterval`)

### Phase 4: pd-api Module (Our max-api)

**Create npm package for better DX:**

```javascript
// pd-api module (ships with [node] external)
const pd = require('pd-api');

// Cleaner than global functions
pd.outlet(0, 'bang');
pd.outlet(1, [440, 0.5]);

// Handler registration
pd.on('float', (inlet, value) => {
    pd.post(`Got ${value} on inlet ${inlet}`);
});

// Properties
pd.post(`Running on ${pd.inlets} inlets, ${pd.outlets} outlets`);
pd.post(`Args: ${pd.args.join(', ')}`);

// Dictionary access (if we implement it)
const dict = pd.getDict('my-data');
dict.set('key', { nested: 'value' });
```

## What Won't Work (Without Full Node.js)

These require the actual Node.js runtime (libuv event loop):

- ❌ Native addons (`.node` files) - Need full Node.js binary
- ❌ Child processes - `child_process.spawn()` needs libuv
- ❌ Native networking - `http`/`https` servers need libuv
- ❌ Worker threads - V8 Isolate management is complex
- ❌ Full `fs` module - Security concerns (could add sandboxed version)

**Workaround:** For these cases, users can:
1. Run actual Node.js alongside PD
2. Communicate via OSC/WebSocket (pd already has externals for this)
3. Use pure-JS alternatives when available

## Success Metrics

### Phase 1 Success: Can install and use pure-JS npm packages

```bash
# In PD patch directory
npm install lodash tone mathjs

# In PD patch
[node lodash-test.js]
```

```javascript
// lodash-test.js
const _ = require('lodash');
const pd = require('pd-api');

pd.on('list', (inlet, arr) => {
    const sorted = _.sortBy(arr);
    pd.outlet(0, sorted);
});
```

### Phase 2-3 Success: Async workflows

```javascript
const pd = require('pd-api');
const fetch = require('node-fetch');  // Popular HTTP client

pd.on('bang', async (inlet) => {
    try {
        const response = await fetch('https://api.example.com/data');
        const data = await response.json();
        pd.outlet(0, data.value);
    } catch (err) {
        pd.error(err.message);
    }
});
```

## Exceeding Max/MSP

**Where we can be BETTER than Max [node.script]:**

1. **✅ No separate process** - Lower latency, no IPC overhead
2. **✅ Synchronous option** - Can do tight timing loops
3. **✅ Lighter weight** - V8 embedded vs full Node.js process
4. **✅ Modern V8** - Always latest engine (Max bundles specific Node version)
5. **✅ Direct API** - No message serialization for internal calls

**Where Max [node.script] is better:**

1. **❌ Stability** - Crashes don't take down Max
2. **❌ True Node.js** - 100% compatibility with native modules
3. **❌ Debugger** - Remote debugging protocol built-in

## Recommended Approach

### Immediate: Create `pd-node` Separate Repo

**Reasons:**
1. Maintains pdjs backward compatibility
2. Clearer semantics: `[js]` vs `[node]` (matches Max naming)
3. Can iterate faster without breaking existing patches
4. Each object can be optimized for its use case

**Implementation:**
```
mganss/pd-node/                    # NEW repository
├── pd-node/
│   ├── node.cpp                   # New external
│   ├── module_resolver.cpp        # npm module resolution
│   ├── async_integration.cpp      # Event loop hooks
│   └── pd-api/                    # Our max-api equivalent
│       ├── package.json
│       ├── index.js
│       └── lib/
├── polyfills/                     # Core module shims
│   ├── path.js
│   ├── events.js
│   └── ...
└── CMakeLists.txt
```

**Later:** Once mature, could merge or keep separate based on community feedback

## Next Steps

1. ✅ Create this research document
2. Create OpenSpec proposal for pd-node Phase 1
3. Set up pd-node repository structure
4. Implement module resolution
5. Test with popular npm packages
6. Document usage patterns
7. Community feedback and iteration

---

**References:**
- Max js object: https://docs.cycling74.com/max8/refpages/js
- Max node.script: https://docs.cycling74.com/max8/refpages/node.script
- Node for Max API: https://docs.cycling74.com/nodeformax/api/
- Node.js module resolution: https://nodejs.org/api/modules.html#modules_all_together
- pdjs source: `~/Code/github.com/mganss/pdjs/pdjs/js.cpp`
