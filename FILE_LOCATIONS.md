# File Locations - Where Everything Is Installed

## Pure Data (Pd-0.54-1)

### Main External Files
```
~/Documents/Pd/externals/
├── node.pd_darwin          ← The actual external (53KB)
├── node-help.pd            ← Help file
└── pd-node/                ← Support files directory
    ├── pd-api/             ← npm package for JavaScript
    │   ├── package.json
    │   ├── index.js
    │   └── index.d.ts
    └── examples/           ← Example scripts
        ├── hello.js
        └── hello.ts
```

**Key Point:** Pure Data looks for `node.pd_darwin` directly in `~/Documents/Pd/externals/`

### Why This Structure?

Pure Data **does not search subdirectories** by default. That's why we have:
- **Binary and help in root:** `~/Documents/Pd/externals/node.pd_darwin`
- **Support files in subfolder:** `~/Documents/Pd/externals/pd-node/`

## Plugdata

### External Files
```
~/Library/Application Support/plugdata/externals/pd-node/
├── node.pd_darwin          ← The actual external (53KB)
├── node-help.pd            ← Help file
├── pd-api/                 ← npm package
│   ├── package.json
│   ├── index.js
│   └── index.d.ts
└── examples/               ← Example scripts
    ├── hello.js
    └── hello.ts
```

**Key Point:** Plugdata handles subdirectories better, so everything can be in `pd-node/` subfolder.

## Source Repository

```
~/Code/github.com/theslyprofessor/pd-node/
├── binaries/
│   └── arm64-macos/
│       ├── node.pd_darwin      ← Built from source
│       ├── node-help.pd
│       └── pd-api/
├── node/
│   ├── node.cpp                ← C++ source
│   ├── runtime_detector.cpp
│   └── ipc_bridge.cpp
├── build/                      ← CMake build directory
├── install.sh                  ← Installer script
└── examples/
```

## Installation Flow

```
Source Repository
    ↓
./install.sh copies to:
    ↓
    ├→ Pure Data: ~/Documents/Pd/externals/
    └→ Plugdata: ~/Library/Application Support/plugdata/externals/pd-node/
```

## Verify Installation

### For Pure Data:
```bash
# Check main external
ls -lh ~/Documents/Pd/externals/node.pd_darwin

# Check help file
ls -lh ~/Documents/Pd/externals/node-help.pd

# Check support files
ls ~/Documents/Pd/externals/pd-node/
```

### For Plugdata:
```bash
# Check all files
ls -lh ~/Library/Application\ Support/plugdata/externals/pd-node/
```

## Common Issues

### "couldn't create" Error

**Cause:** Files not in the right location or Pure Data not restarted

**Check:**
```bash
# This MUST exist and be executable:
file ~/Documents/Pd/externals/node.pd_darwin
# Should output: Mach-O 64-bit dynamically linked shared library arm64

# Check permissions:
ls -l ~/Documents/Pd/externals/node.pd_darwin
# Should show: -rwxr-xr-x (executable)
```

**Fix:**
```bash
cd ~/Code/github.com/theslyprofessor/pd-node
./install.sh
# Then restart Pure Data!
```

### Wrong Subdirectory

**Wrong (won't work):**
```
~/Documents/Pd/externals/pd-node/node.pd_darwin  ← Too deep!
```

**Right (will work):**
```
~/Documents/Pd/externals/node.pd_darwin  ← Correct!
```

## How Pure Data Finds Externals

1. Pure Data starts up
2. Reads `~/Documents/Pd/externals/` directory
3. Looks for `*.pd_darwin` files **in that directory only** (not subdirectories)
4. When you create `[node]`, it loads `node.pd_darwin`

This is why the installer puts the binary at:
```
~/Documents/Pd/externals/node.pd_darwin
```

NOT at:
```
~/Documents/Pd/externals/pd-node/node.pd_darwin  ← Won't be found!
```

## Manual Installation

If `./install.sh` doesn't work, copy manually:

### Pure Data:
```bash
cd ~/Code/github.com/theslyprofessor/pd-node

# Copy to root of externals (IMPORTANT!)
cp binaries/arm64-macos/node.pd_darwin ~/Documents/Pd/externals/
cp binaries/arm64-macos/node-help.pd ~/Documents/Pd/externals/

# Copy support files to subfolder
mkdir -p ~/Documents/Pd/externals/pd-node
cp -r binaries/arm64-macos/pd-api ~/Documents/Pd/externals/pd-node/
cp -r examples ~/Documents/Pd/externals/pd-node/

# Make executable
chmod +x ~/Documents/Pd/externals/node.pd_darwin
```

### Plugdata:
```bash
cd ~/Code/github.com/theslyprofessor/pd-node

mkdir -p ~/Library/Application\ Support/plugdata/externals/pd-node
cp binaries/arm64-macos/node.pd_darwin ~/Library/Application\ Support/plugdata/externals/pd-node/
cp binaries/arm64-macos/node-help.pd ~/Library/Application\ Support/plugdata/externals/pd-node/
cp -r binaries/arm64-macos/pd-api ~/Library/Application\ Support/plugdata/externals/pd-node/
cp -r examples ~/Library/Application\ Support/plugdata/externals/pd-node/

chmod +x ~/Library/Application\ Support/plugdata/externals/pd-node/node.pd_darwin
```

## After Installation

**CRITICAL:** Restart Pure Data or Plugdata!

1. Close completely (Cmd+Q)
2. Reopen
3. Try creating `[node]` object

Should work now!

---

**TL;DR:** 
- Pure Data: `~/Documents/Pd/externals/node.pd_darwin` (root, not subfolder)
- Plugdata: `~/Library/Application Support/plugdata/externals/pd-node/node.pd_darwin`
- Run `./install.sh` to set up correctly
- **Must restart** Pure Data/Plugdata after installation
