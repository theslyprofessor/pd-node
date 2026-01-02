# Testing pd-node in Plugdata

## Quick Start

### 1. Install the External

Copy the compiled binary to Plugdata's externals directory:

```bash
cd ~/Code/github.com/theslyprofessor/pd-node

# Find Plugdata's externals directory
# On macOS it's usually: ~/Library/Application Support/plugdata/externals/

# Create pd-node directory
mkdir -p ~/Library/Application\ Support/plugdata/externals/pd-node

# Copy binary and help file
cp binaries/arm64-macos/node.pd_darwin ~/Library/Application\ Support/plugdata/externals/pd-node/
cp binaries/arm64-macos/node-help.pd ~/Library/Application\ Support/plugdata/externals/pd-node/

# Copy pd-api package
cp -r binaries/arm64-macos/pd-api ~/Library/Application\ Support/plugdata/externals/pd-node/

# Copy example scripts
cp -r examples ~/Library/Application\ Support/plugdata/externals/pd-node/
```

### 2. Create Test Patch

Open Plugdata and create a new patch with this:

```
┌─────────┐
│ [node]  │ ← Try creating this object
└─────────┘
```

**Current Status:** The object will create, but won't do anything yet because Phase 2 (IPC bridge) isn't implemented.

---

## What Works Now (Phase 1)

✅ **Object creation** - `[node]` object can be instantiated  
✅ **Runtime detection** - Internal detector finds Bun/Node.js  
❌ **Script loading** - Not yet (needs Phase 2)  
❌ **Message passing** - Not yet (needs Phase 2)  

---

## Testing Workflow (After Phase 2)

### Test 1: Basic Console Output

**Patch:**
```
┌──────────────────────┐
│ [node examples/hello.js] │
└──────────────────────┘
```

**Script:** `examples/hello.js`
```javascript
const pd = require('pd-api');
console.log('Hello from pd-node!');

pd.on('bang', () => {
    pd.outlet(0, 'bang received!');
});
```

**Expected:**
- Console shows: "Hello from pd-node!"
- Banging the object outputs "bang received!"

### Test 2: Number Processing

**Patch:**
```
┌─────────┐
│ [50(    │
│  |      │
│ [node examples/multiply.js]
│  |      │
│ [print] │
└─────────┘
```

**Script:** `examples/multiply.js`
```javascript
const pd = require('pd-api');

pd.on('float', (n) => {
    pd.outlet(0, n * 2);
    console.log(`Received ${n}, sent ${n * 2}`);
});
```

**Expected:**
- Input `50` → Output `100`
- Console: "Received 50, sent 100"

### Test 3: npm Package (lodash)

**Setup:**
```bash
# Create a scripts directory
mkdir -p ~/pd-scripts
cd ~/pd-scripts
npm install lodash
```

**Patch:**
```
┌──────────────────┐
│ [3 1 4 1 5 9 2(  │
│  |               │
│ [node ~/pd-scripts/sort.js]
│  |               │
│ [print]          │
└──────────────────┘
```

**Script:** `~/pd-scripts/sort.js`
```javascript
const _ = require('lodash');
const pd = require('pd-api');

pd.on('list', (...numbers) => {
    const sorted = _.sortBy(numbers);
    pd.outlet(0, 'sorted:', ...sorted);
});
```

**Expected:**
- Input: `3 1 4 1 5 9 2`
- Output: `sorted: 1 1 2 3 4 5 9`

### Test 4: TypeScript

**Script:** `~/pd-scripts/typescript.ts`
```typescript
import * as pd from 'pd-api';

interface Point {
    x: number;
    y: number;
}

let count: number = 0;

pd.on('bang', () => {
    count++;
    pd.outlet(0, `TypeScript bang #${count}`);
});

pd.on('list', (...args: any[]) => {
    const point: Point = {
        x: args[0] || 0,
        y: args[1] || 0
    };
    
    const distance = Math.sqrt(point.x ** 2 + point.y ** 2);
    pd.outlet(0, `distance: ${distance.toFixed(2)}`);
});
```

**Expected:**
- TypeScript compiles and runs via Bun
- Bang increments counter
- List `[3 4(` → `distance: 5.00`

---

## Current Limitations (Phase 1)

⚠️ **No script loading yet** - The `[node script.js]` argument is parsed but not executed  
⚠️ **No message handlers** - Messages sent to object are ignored  
⚠️ **No outlets** - JavaScript can't send data back to PD yet  

**Why:** Phase 2 (IPC bridge) needs to be implemented first.

---

## Debugging

### Check if external is found:

1. Open Plugdata
2. Go to **Preferences → Search Paths**
3. Verify this path exists:
   ```
   ~/Library/Application Support/plugdata/externals/pd-node
   ```

### Check Plugdata console:

When you create `[node]` object, the console should show:
```
[node] pd-node v0.1.0
[node] Bun runtime detected: 1.3.5
[node] Node.js runtime detected: v22.21.1
```

**If you see errors:**
- "can't create" → External not in search path
- No runtime message → Runtime detection failed
- "unknown option" → Wrong Pure Data version

### Verify runtime detection:

```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./test_detector
```

Should output:
```
Testing runtime detector...
=================================

pd-node runtime detection:
  Bun: 1.3.5 (/Users/ntiruviluamala/.nix-profile/bin/bun)
  Node.js: v22.21.1 (/Users/ntiruviluamala/.nix-profile/bin/node)

Runtime for test.js: Bun
Runtime for test.ts: Bun
```

---

## Manual Testing (Phase 1 Verification)

You can verify the external loads without Phase 2 by checking object creation:

**Test Patch:**
```
┌──────────┐
│ [node]   │ ← Should create successfully
└──────────┘

┌────────────────────┐
│ [node test.js]     │ ← Should create with argument
└────────────────────┘
```

**Expected behavior:**
- Object creates without errors
- Help patch opens (right-click → Help)
- Console shows initialization message

**Check help patch:**
Right-click `[node]` → **Help** should open `node-help.pd`

---

## Installation Script

Save this as `install-plugdata.sh`:

```bash
#!/bin/bash
# Install pd-node to Plugdata

set -e

REPO_DIR="$HOME/Code/github.com/theslyprofessor/pd-node"
PLUGDATA_EXT="$HOME/Library/Application Support/plugdata/externals/pd-node"

echo "Installing pd-node to Plugdata..."

# Create directory
mkdir -p "$PLUGDATA_EXT"

# Copy files
cp "$REPO_DIR/binaries/arm64-macos/node.pd_darwin" "$PLUGDATA_EXT/"
cp "$REPO_DIR/binaries/arm64-macos/node-help.pd" "$PLUGDATA_EXT/"
cp -r "$REPO_DIR/binaries/arm64-macos/pd-api" "$PLUGDATA_EXT/"
cp -r "$REPO_DIR/examples" "$PLUGDATA_EXT/"

echo "✅ Installed to: $PLUGDATA_EXT"
echo ""
echo "Next steps:"
echo "1. Open Plugdata"
echo "2. Create a new patch"
echo "3. Create object: [node]"
echo "4. Check console for initialization message"
echo ""
echo "⚠️  Note: Script execution not working yet (Phase 2 needed)"
```

Make it executable:
```bash
chmod +x install-plugdata.sh
./install-plugdata.sh
```

---

## When Phase 2 is Complete

After implementing IPC bridge, re-run the installation script and test:

```
┌─────────┐
│ [bang]  │
│  |      │
│ [node examples/hello.js]
│  |      │
│ [print] │
└─────────┘
```

Should output: `hello from JavaScript!`

---

## Troubleshooting

### "can't create" error

**Fix:** Add search path in Plugdata preferences:
```
Preferences → Search Paths → Add:
~/Library/Application Support/plugdata/externals/pd-node
```

### Runtime not detected

**Check:** Do you have Bun or Node.js installed?
```bash
which bun
which node
```

**Install Bun:**
```bash
curl -fsSL https://bun.sh/install | bash
```

### Script doesn't execute

**Expected** - Phase 2 (IPC bridge) not implemented yet. The external creates but doesn't spawn processes.

### Permission denied

```bash
# Make binary executable
chmod +x ~/Library/Application\ Support/plugdata/externals/pd-node/node.pd_darwin
```

---

## Alternative: Test in Pure Data (vanilla)

If you have Pure Data installed:

```bash
# Install to PD externals
mkdir -p ~/Documents/Pd/externals/pd-node
cp binaries/arm64-macos/node.pd_darwin ~/Documents/Pd/externals/pd-node/
cp binaries/arm64-macos/node-help.pd ~/Documents/Pd/externals/pd-node/
cp -r binaries/arm64-macos/pd-api ~/Documents/Pd/externals/pd-node/
```

Then launch Pure Data and create `[node]` object.

---

## Next: Implement Phase 2

Once Phase 2 (IPC bridge) is complete:
1. Rebuild: `cd build && make -j4`
2. Reinstall: `./install-plugdata.sh`
3. Test: Open Plugdata and try `[node examples/hello.js]`
4. Should see console output and working message passing!

