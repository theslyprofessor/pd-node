# Next Steps for pd-node Implementation

## What We've Accomplished ✅

1. **Repository Structure** - Clean organization following best practices
2. **README.md** - Comprehensive documentation with usage examples
3. **pd-api Package** - Complete npm package with TypeScript definitions
4. **Research Documents** - Deep analysis of Max/MSP comparison and runtime strategy
5. **OpenSpec Proposal** - Detailed implementation roadmap

## Immediate Next Steps (Phase 1)

### 1. Set Up Build System (Week 1) ✅ COMPLETE

- [x] Create `CMakeLists.txt` based on pdjs structure
- [x] Add pd.build as git submodule
- [x] Set up cross-platform builds (macOS/Linux/Windows)
- [ ] Create CI/CD with GitHub Actions

**Status:** Binary compiles successfully! `node.pd_darwin` (53KB) built for arm64-macos.

### 2. Implement Runtime Detection (Week 1-2) ✅ COMPLETE

**File:** `node/runtime_detector.cpp`

- [x] Detect Bun in system PATH
- [x] Detect Node.js in system PATH  
- [x] File extension detection (.js vs .ts)
- [x] Runtime selection logic
- [x] User-friendly error messages

**Verified working:**
```
Bun: 1.3.5 (/Users/ntiruviluamala/.nix-profile/bin/bun)
Node.js: v22.21.1 (/Users/ntiruviluamala/.nix-profile/bin/node)
Runtime for test.js: Bun
Runtime for test.ts: Bun
```

### 3. Create IPC Bridge (Week 2-3)

**Files:** `node/node.cpp`, `node/ipc_bridge.cpp`

- [ ] Spawn Bun/Node.js process
- [ ] stdin/stdout communication protocol
- [ ] JSON message passing
- [ ] Error handling and recovery
- [ ] Process lifecycle management

### 4. Implement pd-api C++ Integration (Week 3)

**File:** `node/pd_api_bridge.cpp`

- [ ] Inject `__pd_internal__` global into JavaScript
- [ ] Map `pd.outlet()` to PD outlet functions
- [ ] Map `pd.post()` to pd_post()
- [ ] Handler registration system
- [ ] Message dispatch from C++ to JavaScript

### 5. Create Example Patches (Week 4)

**Directory:** `examples/`

- [ ] 01-npm-packages: lodash, ramda demos
- [ ] 02-async-fetch: Web API example
- [ ] 03-websocket-server: Real-time communication
- [ ] 04-ai-integration: Using AI libraries

### 6. Testing & Documentation (Week 4)

- [ ] Unit tests for runtime detection
- [ ] Integration tests with real PD patches
- [ ] Installation guide for macOS/Linux/Windows
- [ ] Troubleshooting guide
- [ ] Video tutorial (optional)

## Technical Architecture Overview

```
[node script.ts] (PD object)
       ↓
  node.cpp (C++ external)
       ↓
  Runtime Detector
    ├─ Check .ts extension → Requires Bun
    ├─ which bun → Found! Use Bun
    └─ else → Error: "Install Bun"
       ↓
  Spawn Bun Process
       ↓
  IPC Bridge (stdin/stdout)
    ├─ C++ → JSON → Bun: {type: "message", inlet: 0, ...}
    └─ Bun → JSON → C++: {type: "outlet", outlet: 0, ...}
       ↓
  script.ts executes
    ├─ require('pd-api') gets __pd_internal__
    └─ pd.on('bang', ...) registers handler
       ↓
  User bangs [node] object
       ↓
  C++ sends JSON to Bun
       ↓
  Bun calls handler
       ↓
  Handler calls pd.outlet(0, 'result')
       ↓
  Bun sends JSON to C++
       ↓
  C++ calls outlet_anything()
       ↓
  Result appears in PD patch!
```

## Key Files to Create

### C++ Source

```
node/
├── node.cpp              # Main external entry point
├── runtime_detector.cpp  # Bun/Node detection
├── ipc_bridge.cpp        # Process spawning & communication
├── pd_api_bridge.cpp     # pd-api C++ implementation
└── json_parser.cpp       # Lightweight JSON for IPC
```

### Build System

```
CMakeLists.txt            # Main build file
.github/
└── workflows/
    └── build.yml         # CI/CD
```

### Examples

```
examples/
├── 01-npm-packages/
│   ├── lodash-demo.pd
│   ├── lodash-demo.js
│   └── package.json
├── 02-async-fetch/
│   ├── fetch-api.pd
│   └── fetch-api.ts      # TypeScript!
└── ...
```

## Dependencies to Add

### Build Dependencies

- **pd.build** - PD external framework (git submodule)
- **nlohmann/json** - C++ JSON library (optional, or write minimal parser)

### Runtime Dependencies (User Installs)

- **Bun** (recommended) or **Node.js**

## Resource Links

- **pdjs source**: `~/Code/github.com/mganss/pdjs` - Reference implementation
- **pd.build**: https://github.com/pierreguillot/pd.build
- **Max node.script**: https://docs.cycling74.com/max8/refpages/node.script
- **Bun docs**: https://bun.sh/docs

## Success Metrics

### Phase 1 Complete When:

- ✅ `[node hello.js]` works with Bun
- ✅ `[node hello.js]` works with Node.js (fallback)
- ✅ `[node script.ts]` works with Bun (TypeScript)
- ✅ `bun install lodash` → `require('lodash')` works
- ✅ `pd.on('bang')` handlers execute
- ✅ `pd.outlet(0, value)` sends to PD
- ✅ Builds on macOS, Linux, Windows
- ✅ <1MB external size

## Commands to Run

### Initialize Git

```bash
cd ~/Code/github.com/theslyprofessor/pd-node
git add .
git commit -m "Initial commit: pd-node structure and pd-api package"
git branch -M main
gh repo create theslyprofessor/pd-node --public --source=. --remote=origin
git push -u origin main
```

### Test pd-api Package

```bash
cd pd-api
bun test  # (once we write tests)
```

### Build (Once CMakeLists.txt is ready)

```bash
mkdir build && cd build
cmake ..
make
# Result: pd-node.pd_darwin
```

## Timeline Estimate

- **Week 1**: Build system + runtime detection
- **Week 2**: IPC bridge implementation
- **Week 3**: pd-api C++ integration
- **Week 4**: Examples + testing + docs
- **Week 5**: Beta release + community feedback

**Target: 5 weeks to working beta** 🎯

## Questions to Answer During Development

1. **IPC Protocol**: JSON over stdin/stdout or binary protocol?
2. **Error Handling**: How to show JavaScript errors in PD console?
3. **Hot Reload**: Should we support reloading scripts without recreating object?
4. **Module Caching**: Cache `require()` across reloads?
5. **TypeScript**: Should we detect and warn about missing Bun for .ts files?

## The Vision

```
# 5 weeks from now, users can do this:

cd ~/my-pd-patch
bun install tone lodash

# Create patch.pd with [node synth.ts]

# synth.ts
import { pd } from 'pd-api';
import * as Tone from 'tone';

const synth = new Tone.Synth().toDestination();

pd.on('list', async ([freq, dur]: number[]) => {
    await synth.triggerAttackRelease(freq, dur);
    pd.outlet(0, 'done');
});

# IT JUST WORKS! 🎉
```

---

**Let's build this!** 🚀
