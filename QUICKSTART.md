# pd-node Quick Start Guide

## TL;DR - Test in Plugdata Now

```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./install-plugdata.sh
open /Applications/plugdata.app
```

Then in Plugdata:
1. Create new patch
2. Type `[node]` and press Enter
3. Right-click → Help to see help file

**Current status:** Object creates ✅ but scripts don't execute yet ⚠️ (Phase 2 needed)

---

## What Works Right Now (Phase 1)

✅ External compiles (53KB binary)  
✅ Runtime detection (finds Bun + Node.js)  
✅ Object creation in Plugdata  
✅ Help file  

❌ Script execution (needs Phase 2)  
❌ Message passing (needs Phase 2)  
❌ npm packages (needs Phase 2)  

---

## Installation

### Option 1: Automatic (Recommended)
```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./install-plugdata.sh
```

### Option 2: Manual
```bash
# Copy to Plugdata externals
mkdir -p ~/Library/Application\ Support/plugdata/externals/pd-node
cp binaries/arm64-macos/node.pd_darwin ~/Library/Application\ Support/plugdata/externals/pd-node/
cp binaries/arm64-macos/node-help.pd ~/Library/Application\ Support/plugdata/externals/pd-node/
cp -r binaries/arm64-macos/pd-api ~/Library/Application\ Support/plugdata/externals/pd-node/
```

---

## Testing Phase 1

Open `test-patch.pd` in Plugdata:
```bash
open -a plugdata ~/Code/github.com/theslyprofessor/pd-node/test-patch.pd
```

Or manually create objects:
- `[node]` - Basic object
- `[node examples/hello.js]` - With script path
- `[node examples/hello.ts]` - TypeScript file

**Expected:** Objects create without errors

**Not working yet:** Script doesn't execute (console.log won't appear)

---

## Development Workflow

### Rebuild after code changes:
```bash
cd ~/Code/github.com/theslyprofessor/pd-node/build
make -j4
```

### Reinstall to Plugdata:
```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./install-plugdata.sh
```

### Restart Plugdata to reload external

---

## Verify Runtime Detection

```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./test_detector
```

Should output:
```
Bun: 1.3.5 (/Users/ntiruviluamala/.nix-profile/bin/bun)
Node.js: v22.21.1 (/Users/ntiruviluamala/.nix-profile/bin/node)
```

---

## When Phase 2 is Complete

After IPC bridge implementation, you'll be able to:

**Test 1: Console output**
```javascript
// hello.js
console.log('Hello from Bun!');
```

**Test 2: Message handling**
```javascript
// multiply.js
const pd = require('pd-api');
pd.on('float', (n) => pd.outlet(0, n * 2));
```

**Test 3: npm packages**
```javascript
// sort.js
const _ = require('lodash');
const pd = require('pd-api');
pd.on('list', (...args) => pd.outlet(0, ..._.sortBy(args)));
```

**Test 4: TypeScript**
```typescript
// point.ts
import * as pd from 'pd-api';
interface Point { x: number; y: number; }
// ... TypeScript code
```

---

## File Locations

| What | Where |
|------|-------|
| **Repository** | `~/Code/github.com/theslyprofessor/pd-node` |
| **Binary** | `binaries/arm64-macos/node.pd_darwin` |
| **Plugdata install** | `~/Library/Application Support/plugdata/externals/pd-node/` |
| **Test patch** | `test-patch.pd` |
| **Examples** | `examples/hello.js`, `examples/hello.ts` |

---

## Documentation

- `README.md` - Project overview
- `STATUS.md` - Current development status
- `TESTING.md` - Comprehensive testing guide (you are here is the quick version)
- `NEXT_STEPS.md` - Implementation roadmap
- `.openspec/proposals/phase-2-ipc-bridge.md` - Next phase plan

---

## Troubleshooting

### "can't create" in Plugdata
→ Run `./install-plugdata.sh` again

### No runtime detected
→ Install Bun: `curl -fsSL https://bun.sh/install | bash`

### Script doesn't run
→ Expected! Phase 2 not implemented yet

### Permission error
→ `chmod +x ~/Library/Application\ Support/plugdata/externals/pd-node/node.pd_darwin`

---

## Next Steps

**For users:** Wait for Phase 2 (IPC bridge) - scripts will work then!

**For developers:** Implement `.openspec/proposals/phase-2-ipc-bridge.md`
1. Create `node/ipc_bridge.cpp`
2. Implement process spawning (fork + exec)
3. Add JSON message passing
4. Wire up pd-api dispatch
5. Test with `examples/hello.js`

---

## Questions?

**GitHub:** https://github.com/theslyprofessor/pd-node  
**Issues:** https://github.com/theslyprofessor/pd-node/issues  

**Current phase:** ✅ Phase 1 complete | 🚧 Phase 2 next
