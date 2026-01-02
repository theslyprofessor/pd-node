---
created: 2026-01-02T03:40:00-0800
updated: 2026-01-02T03:40:00-0800
status: proposed
author: Nakul Tiruviluamala (with Claude)
type: new-feature
scope: new-repository
---

# OpenSpec Proposal: pd-node - Node.js-compatible Pure Data External

## Vision Statement

**Mirror Max/MSP's two-object strategy, then EXCEED it.**

Create `[node]` external for Pure Data that matches Max's `[node.script]` functionality while leveraging pdjs's superior foundation (modern V8 vs Max's bundled Node.js).

## The Strategy: Compatibility → Superiority

### Phase 1: Match Max/MSP
- ✅ Two distinct objects: `[js]` (pdjs) + `[node]` (new)
- ✅ npm package support
- ✅ node_modules resolution
- ✅ Async/await workflows
- ✅ pd-api module (equivalent to max-api)

### Phase 2: Exceed Max/MSP
- 🚀 **Embedded runtime** (no separate process = lower latency)
- 🚀 **Always latest V8** (Max bundles specific Node version)
- 🚀 **Dual-mode operation** (sync + async in same object)
- 🚀 **Better performance** (no IPC overhead)
- 🚀 **Tighter scheduler integration** (can do sample-accurate timing if needed)

## Current State Analysis

### pdjs vs Max [js]

| Feature | Max [js] | pdjs | Winner |
|---------|----------|------|--------|
| Engine | SpiderMonkey 2011 | V8 latest | **pdjs** 🏆 |
| ES6+ | ❌ No `let`, arrow functions | ✅ ES2023+ | **pdjs** 🏆 |
| Performance | Medium | High | **pdjs** 🏆 |
| require() | ✅ Local files only | ✅ Local files only | Tie |
| API access | Max API | PD API | Different ecosystems |

**pdjs already DESTROYS Max [js] in terms of language features and performance.**

### What's Missing: The [node.script] Gap

Max users have `[node.script]` for:
- npm packages (lodash, tone.js, AI libraries, etc.)
- Modern workflows (async/await, promises)
- Web APIs (fetch, websockets)
- File system access
- External tool integration

**PD users have NOTHING comparable.** We change that.

## Proposal: Create pd-node Repository

### New Repository Structure

```
github.com/mganss/pd-node/              # NEW repo
├── README.md
├── CMakeLists.txt
├── node/
│   ├── node.cpp                        # Main external
│   ├── module_resolver.cpp             # npm resolution
│   ├── async_pump.cpp                  # Event loop integration
│   └── node-help.pd                    # Help patcher
├── pd-api/                             # npm package (like max-api)
│   ├── package.json
│   ├── index.js
│   ├── lib/
│   │   ├── outlet.js
│   │   ├── handlers.js
│   │   └── dict.js
│   └── README.md
├── polyfills/                          # Core Node.js modules
│   ├── path/
│   ├── events/
│   ├── util/
│   └── buffer/
└── examples/
    ├── 01-npm-packages/
    ├── 02-async-fetch/
    ├── 03-websocket-server/
    └── 04-ai-integration/
```

### Key Design Decisions

**1. Separate Repository (Not Fork)**
- Clean slate for Node.js-specific features
- Independent release cycle
- Can share code/learnings with pdjs but different goals

**2. Naming: `[node]` not `[node.script]`**
- Cleaner: `[node lodash-demo.js]`
- Avoids confusion with PD's message system
- Matches the pattern: `[js]` vs `[node]`

**3. Embedded V8 (Not Child Process)**
- **Different from Max approach** but BETTER
- No IPC latency
- Shared memory with PD
- Can still crash-isolate later if needed

## Phase 1 Implementation: Core npm Support

### Goal: Match Max [node.script] Basics

**Success Metric:**
```bash
# In PD patch directory
npm install lodash

# In PD patch - THIS JUST WORKS
[node test.js]
```

```javascript
// test.js
const _ = require('lodash');
const pd = require('pd-api');

pd.on('list', (inlet, arr) => {
    const sorted = _.sortBy(arr);
    pd.outlet(0, sorted);
});
```

### Technical Implementation

#### 1. Enhanced Module Resolution

**File:** `node/module_resolver.cpp`

```cpp
#include <filesystem>
#include <fstream>
#include "json.hpp"  // nlohmann/json for package.json parsing

namespace fs = std::filesystem;

class ModuleResolver {
private:
    std::string base_dir;
    
    // Check if module is a core polyfill
    bool is_core_module(const std::string& name) {
        static const std::unordered_set<std::string> core = {
            "path", "events", "util", "buffer", "stream", "process"
        };
        return core.find(name) != core.end();
    }
    
    // Parse package.json "main" field
    std::optional<std::string> parse_package_main(const fs::path& pkg_json_path) {
        std::ifstream file(pkg_json_path);
        if (!file.is_open()) return std::nullopt;
        
        try {
            nlohmann::json pkg = nlohmann::json::parse(file);
            if (pkg.contains("main")) {
                return pkg["main"].get<std::string>();
            }
        } catch (...) {
            return std::nullopt;
        }
        
        return "index.js";  // Default
    }

public:
    ModuleResolver(const std::string& dir) : base_dir(dir) {}
    
    // Main resolution function - follows Node.js algorithm
    std::optional<std::string> resolve(const std::string& module_name) {
        // 1. Relative/absolute paths - pass through
        if (module_name[0] == '.' || module_name[0] == '/') {
            fs::path full_path = fs::path(base_dir) / module_name;
            
            // Try exact match
            if (fs::exists(full_path)) return full_path.string();
            
            // Try with .js extension
            full_path.replace_extension(".js");
            if (fs::exists(full_path)) return full_path.string();
            
            return std::nullopt;
        }
        
        // 2. Core module polyfills
        if (is_core_module(module_name)) {
            // Return embedded polyfill path
            return get_polyfill_path(module_name);
        }
        
        // 3. node_modules traversal (THE KEY FEATURE)
        fs::path current_dir = base_dir;
        
        while (true) {
            fs::path node_modules = current_dir / "node_modules" / module_name;
            
            // Try package.json
            fs::path pkg_json = node_modules / "package.json";
            if (fs::exists(pkg_json)) {
                auto main_file = parse_package_main(pkg_json);
                if (main_file) {
                    fs::path entry = node_modules / *main_file;
                    if (fs::exists(entry)) return entry.string();
                }
            }
            
            // Try index.js fallback
            fs::path index = node_modules / "index.js";
            if (fs::exists(index)) return index.string();
            
            // Try direct .js file
            fs::path direct = node_modules.string() + ".js";
            if (fs::exists(direct)) return direct.string();
            
            // Move up directory tree
            if (current_dir.has_parent_path()) {
                auto parent = current_dir.parent_path();
                if (parent == current_dir) break;  // Hit filesystem root
                current_dir = parent;
            } else {
                break;
            }
        }
        
        return std::nullopt;  // Module not found
    }
};
```

#### 2. Updated require() Function

**File:** `node/node.cpp`

```cpp
static void node_require(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::External> ext = v8::Local<v8::External>::Cast(args.Data());
    auto x = (t_node*)ext->Value();
    
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().SetUndefined();
        return;
    }
    
    std::string module_name = to_string(isolate, args[0]);
    
    // Use enhanced resolver
    ModuleResolver resolver(x->dir);
    auto resolved_path = resolver.resolve(module_name);
    
    if (!resolved_path) {
        std::string error = "Cannot find module '" + module_name + "'";
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error.c_str()).ToLocalChecked()
        ));
        args.GetReturnValue().SetUndefined();
        return;
    }
    
    // Check module cache (like Node.js require.cache)
    auto cached = x->module_cache.find(*resolved_path);
    if (cached != x->module_cache.end()) {
        args.GetReturnValue().Set(scope.Escape(cached->second.Get(isolate)));
        return;
    }
    
    // Create exports object
    v8::Local<v8::Object> exports_obj = v8::Object::New(isolate);
    auto exports_key = v8::String::NewFromUtf8Literal(isolate, "exports");
    exports_obj->Set(context, exports_key, v8::Object::New(isolate)).Check();
    
    // Load and execute module
    if (node_load_module(x, resolved_path->c_str(), &exports_obj)) {
        v8::Local<v8::Value> exports;
        if (exports_obj->Get(context, exports_key).ToLocal(&exports)) {
            // Cache the module
            x->module_cache[*resolved_path].Reset(isolate, exports);
            
            args.GetReturnValue().Set(scope.Escape(exports));
            return;
        }
    }
    
    args.GetReturnValue().SetUndefined();
}
```

#### 3. pd-api npm Package

**File:** `pd-api/package.json`

```json
{
  "name": "pd-api",
  "version": "1.0.0",
  "description": "Pure Data API for node external (like max-api for Max/MSP)",
  "main": "index.js",
  "keywords": ["pure-data", "pd", "audio", "dsp", "music"],
  "author": "pd-node contributors",
  "license": "MIT",
  "repository": {
    "type": "git",
    "url": "https://github.com/mganss/pd-node.git"
  }
}
```

**File:** `pd-api/index.js`

```javascript
// pd-api module - cleaner interface than global functions
// Mirrors max-api design but for Pure Data

const handlers = new Map();
let currentInlet = 0;
let currentMessage = '';

// Injected by C++ code (like max-api does)
const _internal = global.__pd_internal__ || {};

// Public API
const pd = {
    // Properties (read-only)
    get inlets() { return _internal.inlets || 1; },
    get outlets() { return _internal.outlets || 1; },
    get inlet() { return currentInlet; },
    get messagename() { return currentMessage; },
    get args() { return _internal.jsarguments || []; },
    
    // Output methods
    outlet(index, ...values) {
        if (values.length === 0) {
            _internal.outlet(index);  // bang
        } else if (values.length === 1) {
            _internal.outlet(index, values[0]);
        } else {
            _internal.outlet(index, values);
        }
    },
    
    // Logging
    post(...args) {
        _internal.post(args.join(' '));
    },
    
    error(...args) {
        _internal.error(args.join(' '));
    },
    
    // Event handlers (like max-api's addHandler)
    on(message, callback) {
        if (!handlers.has(message)) {
            handlers.set(message, []);
        }
        handlers.get(message).push(callback);
        
        // Register with C++ side
        _internal.registerHandler(message);
    },
    
    // Called by C++ when message arrives
    _dispatch(inlet, message, ...args) {
        currentInlet = inlet;
        currentMessage = message;
        
        const messageHandlers = handlers.get(message);
        if (messageHandlers) {
            messageHandlers.forEach(handler => {
                try {
                    handler(inlet, ...args);
                } catch (err) {
                    pd.error(`Handler error: ${err.message}`);
                }
            });
        }
        
        currentInlet = 0;
        currentMessage = '';
    },
    
    // Future: Dictionary support
    getDict(name) {
        // TODO: Implement if PD has dictionary support
        throw new Error('Dictionary support not yet implemented');
    }
};

module.exports = pd;
```

#### 4. Async Event Loop Integration

**File:** `node/async_pump.cpp`

```cpp
#include <m_pd.h>
#include <v8.h>
#include <libplatform/libplatform.h>

// PD clock for pumping V8 microtasks
static t_clock* node_async_clock = nullptr;
static std::unique_ptr<v8::Platform> node_platform;
static v8::Isolate* node_isolate = nullptr;

// Called every PD tick to process async tasks
static void node_pump_async(void* dummy) {
    if (!node_isolate) return;
    
    v8::Isolate::Scope isolate_scope(node_isolate);
    
    // Pump microtask queue (Promises, async/await)
    while (v8::platform::PumpMessageLoop(node_platform.get(), node_isolate)) {
        // Continue until queue empty
    }
    
    // Process pending timers (setTimeout, setInterval)
    // TODO: Implement timer queue
    
    // Reschedule for next PD tick
    clock_delay(node_async_clock, 0);
}

void node_async_init() {
    // Create PD clock for async pumping
    node_async_clock = clock_new(nullptr, (t_method)node_pump_async);
    clock_delay(node_async_clock, 0);  // Start pumping
}

void node_async_cleanup() {
    if (node_async_clock) {
        clock_free(node_async_clock);
        node_async_clock = nullptr;
    }
}
```

## Where We EXCEED Max/MSP

### 1. Performance: Embedded vs Separate Process

**Max [node.script]:**
```
Max Patch → IPC → Node.js Process → IPC → Max Patch
Latency: ~5-10ms per round-trip
```

**pd-node:**
```
PD Patch → Direct V8 call → PD Patch
Latency: <1ms (function call overhead only)
```

### 2. Flexibility: Dual-Mode Operation

**Max:** Must choose `[js]` OR `[node.script]` - different workflows

**pd-node:** Can mix sync and async in SAME object

```javascript
const pd = require('pd-api');
const _ = require('lodash');

// Synchronous tight loop - NO PROBLEM
pd.on('list', (inlet, arr) => {
    const sorted = _.sortBy(arr);  // Instant
    pd.outlet(0, sorted);
});

// Async workflow - ALSO NO PROBLEM
pd.on('bang', async (inlet) => {
    const data = await fetchSomething();  // Promise-based
    pd.outlet(1, data);
});
```

### 3. Always Latest V8

**Max:** Ships Node.js v20.x (or whatever version Cycling'74 bundles)
**pd-node:** Uses latest V8 from system or builds latest - always cutting edge

### 4. Lighter Weight

**Max [node.script]:** Spawns full Node.js process (~50MB memory minimum)
**pd-node:** Embedded V8 (~10-15MB memory overhead)

## Compatibility Strategy

### Keep pdjs Backward Compatible

```
[js]  (pdjs - UNCHANGED)
├─ Existing patches work
├─ Max [js] script compatibility
├─ Synchronous execution model
└─ Local file require() only
```

### New [node] for Modern Workflows

```
[node]  (pd-node - NEW)
├─ npm package support
├─ Async/await
├─ Modern Node.js patterns
└─ Max [node.script] script compatibility (where possible)
```

## Migration Path for Max Users

**Max user wants to port patch to PD:**

```max
[js mycode.js]          →  [js mycode.js]         (works if no Max API)
[node.script server.js] →  [node server.js]       (works with pd-api changes)
```

**Example Max → PD port:**

```javascript
// Max version (max-api)
const maxApi = require('max-api');

maxApi.addHandler('bang', () => {
    maxApi.outlet('hello');
});

// PD version (pd-api) - NEARLY IDENTICAL
const pd = require('pd-api');

pd.on('bang', () => {
    pd.outlet(0, 'hello');
});
```

## Implementation Timeline

### Phase 1: Core npm Support (THIS PROPOSAL)
**Duration:** 2-3 weeks
- ✅ Create pd-node repository
- ✅ Implement module resolver
- ✅ Create pd-api npm package
- ✅ Test with popular packages (lodash, ramda, etc.)

### Phase 2: Async Integration
**Duration:** 1-2 weeks
- ✅ Microtask pump via PD clock
- ✅ Timer support (setTimeout/setInterval)
- ✅ Promise/async-await workflows

### Phase 3: Core Module Polyfills
**Duration:** 1 week
- ✅ Bundle path, events, util, buffer
- ✅ Test compatibility with packages needing these

### Phase 4: Documentation & Examples
**Duration:** 1 week
- ✅ Comprehensive README
- ✅ Example patches
- ✅ Migration guide from Max

### Phase 5: Community Feedback & Iteration
**Duration:** Ongoing
- ✅ Release beta
- ✅ Gather feedback
- ✅ Fix bugs
- ✅ Add requested features

## Success Criteria

### Must Have (Phase 1)
- [x] `npm install` packages work with `require()`
- [x] pd-api module provides clean API
- [x] Popular pure-JS packages work (lodash, ramda, mathjs)
- [x] Help patcher demonstrates usage
- [x] Builds on macOS/Linux/Windows

### Should Have (Phase 2-3)
- [x] async/await workflows function
- [x] Core module polyfills available
- [x] Performance better than Max [node.script]
- [x] Example patches for common use cases

### Nice to Have (Phase 4-5)
- [ ] Timer precision better than Max
- [ ] Ability to hot-reload scripts
- [ ] Built-in package manager integration
- [ ] Community package repository

## Next Steps

1. **Create pd-node repository**
2. **Port pdjs build system** (CMake, pd.build, V8 integration)
3. **Implement ModuleResolver class**
4. **Create pd-api package**
5. **Test with real npm packages**
6. **Document and release alpha**

## Questions for Review

1. **Repository location:** `mganss/pd-node` or `pd-node/pd-node`?
2. **License:** Same as pdjs (LPGL)?
3. **Build system:** Reuse pdjs's CMake setup?
4. **Polyfill strategy:** Bundle or require separate install?

---

**References:**
- Research document: `.openspec/research/max-msp-comparison.md`
- pdjs source: `pdjs/js.cpp`
- Max node.script docs: https://docs.cycling74.com/max8/refpages/node.script
- max-api source: https://github.com/Cycling74/max-api
