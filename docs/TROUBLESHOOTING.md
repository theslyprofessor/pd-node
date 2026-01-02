# Troubleshooting pd-node

## "couldn't create" Error

### Symptoms
```
node
... couldn't create
```

### Causes & Solutions

#### 1. Pure Data/Plugdata Not Restarted
**Solution:** Close and reopen Pure Data/Plugdata after installation

```bash
# Install the external
cd ~/Code/github.com/theslyprofessor/pd-node
./install.sh

# MUST restart Pure Data/Plugdata for it to load!
```

#### 2. External Not in Search Path
**Check:** Verify files are installed

```bash
# For Pure Data:
ls ~/Documents/Pd/externals/pd-node/

# For Plugdata:
ls ~/Library/Application\ Support/plugdata/externals/pd-node/
```

**Should see:**
- `node.pd_darwin` (53KB)
- `node-help.pd`
- `pd-api/`
- `examples/`

**If missing:** Run `./install.sh` again

#### 3. Wrong Architecture
**Check:** Verify you're on arm64 Mac

```bash
uname -m
# Should output: arm64
```

**If x86_64:** Need to rebuild for Intel Mac (not yet supported)

#### 4. Permission Issues
**Fix:** Make binary executable

```bash
chmod +x ~/Documents/Pd/externals/pd-node/node.pd_darwin
chmod +x ~/Library/Application\ Support/plugdata/externals/pd-node/node.pd_darwin
```

---

## Verification Steps

### 1. Check Installation
```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./install.sh
```

Should output:
```
✅ Pure Data (Pd-0.54-1)
✅ Plugdata
✅ Installed to: ...
```

### 2. Restart Application
**CRITICAL:** Pure Data only loads externals at startup!

- Close Pure Data completely (Cmd+Q)
- Reopen Pure Data
- Try creating `[node]` object again

### 3. Check Console
After creating `[node]`, console should show:
```
[node] pd-node v0.1.0
[node] Runtime detected: Bun 1.3.5
```

If you see "couldn't create" → External not loaded

### 4. Manual Path Check (Advanced)
In Pure Data:
1. Go to **Preferences → Path**
2. Add if missing: `~/Documents/Pd/externals/pd-node`
3. Restart Pure Data

---

## Platform-Specific Issues

### macOS Gatekeeper
**Symptom:** "cannot be opened because the developer cannot be verified"

**Fix:**
```bash
xattr -cr ~/Documents/Pd/externals/pd-node/node.pd_darwin
```

### File Not Found
**Check:** Binary exists and is executable

```bash
file ~/Documents/Pd/externals/pd-node/node.pd_darwin
# Should show: Mach-O 64-bit dynamically linked shared library arm64
```

---

## Testing After Install

### Test 1: Object Creation
In Pure Data:
1. Create new patch
2. Put: `[node]`
3. Should create without "couldn't create" error

✅ **Success:** Object box appears  
❌ **Failure:** Red dashed box with error

### Test 2: Help File
1. Right-click `[node]` object
2. Select **Help**
3. Should open `node-help.pd`

✅ **Success:** Help patch opens  
❌ **Failure:** "couldn't find help" error

### Test 3: Runtime Detection
```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./test_detector
```

Should show:
```
Bun: 1.3.5 (...)
Node.js: v22.21.1 (...)
```

---

## Common Mistakes

### ❌ Forgot to Restart Pure Data
External won't load until restart!

### ❌ Wrong Application
Installed to Pure Data but opened Plugdata (or vice versa)

**Solution:** Run `./install.sh` - installs to both

### ❌ Expecting Scripts to Work
Phase 1: Only object creation works  
Phase 2: Script execution (not implemented yet)

### ❌ Using Relative Paths
Pure Data patch is in different directory than script

**Wrong:** `[node hello.js]`  
**Right:** `[node ~/Documents/pd-scripts/hello.js]`

Or put scripts in same directory as patch.

---

## Still Not Working?

### Collect Debug Info
```bash
# 1. Check installation
ls -lh ~/Documents/Pd/externals/pd-node/

# 2. Check binary
file ~/Documents/Pd/externals/pd-node/node.pd_darwin

# 3. Check runtime
cd ~/Code/github.com/theslyprofessor/pd-node
./test_detector

# 4. Check Pure Data version
/Applications/Pd-0.54-1.app/Contents/Resources/bin/pd -version
```

### Report Issue
Include output from above commands at:
https://github.com/theslyprofessor/pd-node/issues

---

## Quick Fix Checklist

- [ ] External installed (`./install.sh`)
- [ ] Pure Data/Plugdata **restarted** (Cmd+Q, reopen)
- [ ] Files exist in externals directory
- [ ] Binary is executable (`chmod +x`)
- [ ] Test patch opens without errors
- [ ] Help file accessible (right-click → Help)

If all checked and still failing → Report issue with debug info

---

## Expected Behavior (Phase 1)

### What Should Work ✅
- Creating `[node]` object
- Help file opens
- No console errors

### What Doesn't Work Yet ⚠️
- Script execution
- Message passing
- Console output from JavaScript
- `require('pd-api')`

**Why:** Phase 2 (IPC bridge) not implemented

---

## Next Steps

After verifying object creation works:
1. Wait for Phase 2 implementation
2. Or help implement IPC bridge (see `.openspec/proposals/phase-2-ipc-bridge.md`)

