# pd-node Development Status

**Last updated:** January 2, 2025

---

## 🎯 Project Goal

Bring npm packages and TypeScript to Pure Data via `[node]` external - like Max/MSP's `[node.script]` but better.

**Key Innovation:** Uses system-installed Bun/Node.js instead of bundling (Max/MSP = 2GB, pd-node < 1MB)

---

## ✅ Phase 1: Foundation (COMPLETE)

### What Works
- ✅ **Repository structure** - Clean organization following pd.build conventions
- ✅ **Build system** - CMake compiles successfully for arm64-macos
- ✅ **Binary output** - `node.pd_darwin` (53KB) ready for Pure Data
- ✅ **Runtime detection** - Automatically finds Bun and Node.js on system
- ✅ **File type detection** - .ts files → Bun required, .js files → Bun preferred
- ✅ **pd-api package** - Complete npm package with TypeScript definitions
- ✅ **Documentation** - Comprehensive README, research docs, implementation plans

### Verified Working
```bash
Runtime Detection Test Results:
  Bun: 1.3.5 (/Users/ntiruviluamala/.nix-profile/bin/bun)
  Node.js: v22.21.1 (/Users/ntiruviluamala/.nix-profile/bin/node)
  
  test.js → Bun runtime
  test.ts → Bun runtime (TypeScript support)
```

### Build Commands
```bash
cd ~/Code/github.com/theslyprofessor/pd-node

# Configure
nix-shell -p cmake gnumake --run "mkdir -p build && cd build && cmake .."

# Compile
nix-shell -p cmake gnumake --run "cd build && make -j4"

# Output: binaries/arm64-macos/node.pd_darwin
```

---

## 🚧 Phase 2: IPC Bridge (IN PROGRESS)

### Goal
Bidirectional communication between C++ external and Bun/Node.js child process.

### Status: Not Yet Started

**Blockers:** None - ready to implement!

### Implementation Plan
See: `.openspec/proposals/phase-2-ipc-bridge.md`

**Key tasks:**
1. Process spawning (fork + exec)
2. Wrapper script injection (sets up pd-api globals)
3. JSON message protocol (stdin/stdout communication)
4. Non-blocking I/O (using PD's clock system)
5. pd-api dispatch mechanism

**Estimated time:** 1-2 weeks

---

## 📋 Current Architecture

```
Pure Data Patch
       ↓
[node hello.js] ← C++ external (node.pd_darwin)
       ↓
RuntimeDetector → Finds Bun/Node.js
       ↓
IPCBridge → Spawns child process (NOT IMPLEMENTED YET)
       ↓
wrapper.js → Injects pd-api globals (NOT IMPLEMENTED YET)
       ↓
hello.js → User's script
       ↓
pd-api → require('pd-api') (COMPLETE, but not wired up yet)
       ↓
JSON messages back to C++ (NOT IMPLEMENTED YET)
       ↓
PD outlets (NOT IMPLEMENTED YET)
```

---

## 🎓 What We Learned

### Why Not Bundle Node.js?
- Max/MSP bundles Node.js → 2GB download
- pd-node detects system runtime → <1MB external
- Users already have Bun/Node.js for other projects
- Easier updates (system package manager handles it)

### Runtime Selection Logic
```cpp
if (file.endsWith(".ts")) {
    return Bun;  // Node.js can't run TypeScript natively
} 
else if (file.endsWith(".js")) {
    if (bunAvailable) return Bun;    // Prefer Bun (faster)
    if (nodeAvailable) return Node;  // Fallback to Node.js
}
```

### Message Protocol (Planned)
- **C++ → JS:** `{"type":"message","inlet":0,"selector":"bang","args":[]}`
- **JS → C++:** `{"type":"outlet","outlet":0,"selector":"float","args":[440]}`
- **Special:** `{"type":"log","message":"hello"}`

---

## 📁 Repository Structure

```
pd-node/
├── binaries/
│   └── arm64-macos/
│       ├── node.pd_darwin      ← Compiled external (53KB)
│       ├── node-help.pd        ← Help patcher
│       └── pd-api/             ← Bundled npm package
├── node/
│   ├── node.cpp                ← Main PD external entry point
│   ├── runtime_detector.cpp    ← ✅ Detects Bun/Node.js
│   ├── runtime_detector.h
│   ├── ipc_bridge.cpp          ← ❌ TODO: Process spawning
│   └── ipc_bridge.h
├── pd-api/
│   ├── package.json
│   ├── index.js                ← ✅ Complete API (not wired yet)
│   ├── index.d.ts              ← TypeScript definitions
│   └── README.md
├── examples/
│   ├── hello.js                ← Test script (JavaScript)
│   └── hello.ts                ← Test script (TypeScript)
├── .openspec/
│   ├── research/
│   │   ├── max-msp-comparison.md
│   │   └── runtime-size-comparison.md
│   └── proposals/
│       ├── PROPOSAL.md
│       └── phase-2-ipc-bridge.md  ← Current implementation plan
├── CMakeLists.txt              ← Build configuration
├── README.md
├── NEXT_STEPS.md
└── STATUS.md                   ← You are here
```

---

## 🧪 Testing Plan (Phase 2)

### Test 1: Console Output
```javascript
// test-console.js
console.log("Hello from Bun!");
```
**Expected:** Message appears in PD console

### Test 2: Bang Handler
```javascript
// test-bang.js
const pd = require('pd-api');
pd.on('bang', () => {
    pd.outlet(0, 'received bang!');
});
```
**Expected:** Banging `[node test-bang.js]` outputs to `[print]`

### Test 3: npm Packages
```javascript
// test-lodash.js
const _ = require('lodash');
const pd = require('pd-api');

pd.on('list', (...args) => {
    pd.outlet(0, 'sorted:', ..._.sortBy(args));
});
```
**Expected:** `[3 1 2]` → `[sorted: 1 2 3]`

### Test 4: TypeScript
```typescript
// test-typescript.ts
import * as pd from 'pd-api';

interface Point { x: number; y: number; }

pd.on('list', (...args: any[]) => {
    const p: Point = { x: args[0], y: args[1] };
    pd.outlet(0, `Point(${p.x}, ${p.y})`);
});
```
**Expected:** TypeScript compiles and runs via Bun

---

## 🔗 Related Projects

- **pdjs** - https://github.com/mganss/pdjs (V8 JavaScript, our inspiration)
- **Max node.script** - What we're emulating
- **port** - https://github.com/thisconnect/port (Different: Node.js controls PD from outside)

---

## 🚀 Next Actions

### Immediate (Today/Tomorrow)
1. Download nlohmann/json header library
2. Create `node/ipc_bridge.h` skeleton
3. Implement `spawn_process()` function
4. Test process spawning with minimal "hello world"

### This Week
5. Create `node/wrapper.js` and embed in C++
6. Implement JSON message passing
7. Wire up `pd.on()` handlers
8. Test with `examples/hello.js`

### Next Week  
9. Add non-blocking I/O polling
10. Test with npm packages (lodash)
11. Verify TypeScript support
12. Clean up error handling

---

## 💡 Key Insights

### Why This Will Work
✅ **Architecture is simple** - Just stdin/stdout JSON, no complex threading  
✅ **Max already did it** - We know this pattern works in audio software  
✅ **Minimal dependencies** - Header-only JSON lib, POSIX pipes  
✅ **Fast iteration** - CMake rebuilds in seconds  

### Why This is Better Than Max
🎯 **Smaller** - 53KB vs 2GB download  
🎯 **System runtime** - Users control Node/Bun version  
🎯 **TypeScript native** - Via Bun, no transpilation needed  
🎯 **Open source** - MIT license, community can contribute  

---

## 📞 Contact

**Project:** https://github.com/theslyprofessor/pd-node  
**Author:** Nakul Tiruviluamala  
**Status:** Active development (Phase 2 starting)

---

**TL;DR:** Phase 1 complete! Binary compiles, runtime detection works. Next: Implement IPC bridge to actually spawn Bun/Node.js and talk to it.
