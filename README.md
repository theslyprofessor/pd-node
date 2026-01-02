# pd-node

**Modern JavaScript & TypeScript for Pure Data**

Bring the full npm ecosystem to Pure Data with native TypeScript support, powered by Bun or Node.js.

---

## 🎯 Vision

**Match Max/MSP's dual JavaScript approach, then EXCEED it.**

Pure Data gets TWO JavaScript objects (like Max/MSP):
- `[js]` - [pdjs](https://github.com/mganss/pdjs) (already exists, modern V8)
- `[node]` - **pd-node** (this project, npm + TypeScript)

## 🚀 Why pd-node?

### vs Max/MSP

| Feature | Max [js] | Max [node.script] | pdjs | **pd-node** |
|---------|----------|-------------------|------|-------------|
| **Engine** | SpiderMonkey 2011 | Node.js v20 (bundled) | V8 latest | Bun/Node (user's choice) |
| **Package Size** | - | **2GB app** | ~15MB | **<1MB** 🏆 |
| **npm packages** | ❌ | ✅ | ❌ | ✅ |
| **TypeScript** | ❌ | Manual config | ❌ | **Native** 🏆 |
| **ES6+** | ❌ | ✅ | ✅ | ✅ |
| **Runtime Updates** | ❌ | Max updates only | ❌ | **User controls** 🏆 |
| **Startup Speed** | Fast | ~500ms | Fast | **~100ms (Bun)** 🏆 |

### Key Advantages

- ✅ **3MB download** vs Max's 2GB
- ✅ **Native TypeScript** - zero config with Bun
- ✅ **Always latest** - user controls runtime version
- ✅ **Fast package installs** - 2-10x faster with Bun
- ✅ **Your choice** - Bun (fast + TypeScript) or Node.js (compatible)

## 📦 Installation

### 1. Install pd-node External

```bash
# Download latest release
# Extract to PD externals folder
~/Documents/Pd/externals/pd-node/
```

### 2. Install Runtime (Choose One)

**Option A: Bun (Recommended)** - Fast, TypeScript native
```bash
curl -fsSL https://bun.sh/install | bash
```

**Option B: Node.js** - Maximum compatibility
```bash
# macOS (with Homebrew)
brew install node

# Or download from https://nodejs.org
```

### 3. Verify Installation

Create a test patch in PD:

```
[node hello.js]
|
[print]
```

Create `hello.js`:
```javascript
const pd = require('pd-api');

pd.on('bang', () => {
    pd.outlet(0, 'Hello from pd-node!');
});
```

Bang the object → Should see "Hello from pd-node!" in console!

## 🎨 Usage

### Basic Example (JavaScript)

```javascript
// metronome.js
const pd = require('pd-api');

let interval;

pd.on('float', (inlet, bpm) => {
    if (interval) clearInterval(interval);
    
    interval = setInterval(() => {
        pd.outlet(0, 'bang');
    }, 60000 / bpm);
});

pd.on('bang', () => {
    if (interval) {
        clearInterval(interval);
        interval = null;
    }
});
```

### TypeScript Example (Bun Only)

```typescript
// synth.ts - Just works with Bun!
import { pd } from 'pd-api';

interface Note {
    pitch: number;
    velocity: number;
}

const processNote = async (note: Note): Promise<void> => {
    pd.outlet(0, [note.pitch, note.velocity]);
    
    // Simulate async processing
    await new Promise(resolve => setTimeout(resolve, 100));
    
    pd.outlet(0, [note.pitch, 0]); // Note off
};

pd.on('list', async (inlet: number, values: number[]) => {
    const note: Note = {
        pitch: values[0],
        velocity: values[1]
    };
    
    await processNote(note);
});
```

### Using npm Packages

```bash
# In your patch directory
bun install lodash tone
# or: npm install lodash tone
```

```javascript
// audio-processor.js
const _ = require('lodash');
const Tone = require('tone');
const pd = require('pd-api');

pd.on('list', (inlet, freqs) => {
    // Use lodash for data processing
    const sorted = _.sortBy(freqs);
    const unique = _.uniq(sorted);
    
    pd.outlet(0, unique);
});
```

## 🎛️ Object Arguments

```
[node script.js]              Auto-detect best runtime
[node script.ts]              TypeScript (requires Bun)
[node --bun script.js]        Force Bun runtime
[node --node script.js]       Force Node.js runtime
[node --help]                 Show runtime info
```

## 📚 pd-api Reference

### Output

```javascript
const pd = require('pd-api');

// Send bang
pd.outlet(0);

// Send single value
pd.outlet(0, 42);
pd.outlet(0, 'hello');

// Send list
pd.outlet(0, [440, 0.5, 1000]);
```

### Input Handlers

```javascript
// Handle bangs
pd.on('bang', (inlet) => {
    pd.post(`Bang on inlet ${inlet}`);
});

// Handle floats
pd.on('float', (inlet, value) => {
    pd.outlet(0, value * 2);
});

// Handle lists
pd.on('list', (inlet, values) => {
    const sum = values.reduce((a, b) => a + b, 0);
    pd.outlet(0, sum);
});

// Handle symbols
pd.on('symbol', (inlet, sym) => {
    pd.post(`Received: ${sym}`);
});

// Handle any message
pd.on('anything', (inlet, selector, args) => {
    pd.post(`${selector}: ${args.join(' ')}`);
});
```

### Properties

```javascript
pd.inlets        // Number of inlets
pd.outlets       // Number of outlets
pd.inlet         // Current inlet (during handler)
pd.messagename   // Current message name
pd.args          // Arguments passed to [node] object
```

### Logging

```javascript
pd.post('Info message');      // Print to PD console
pd.error('Error message');    // Print error to PD console
```

## 🏗️ Project Structure

```
your-pd-patch/
├── main.pd                    # Your PD patch
├── package.json               # npm dependencies
├── node_modules/              # Installed packages (auto)
├── scripts/
│   ├── sequencer.js          # JavaScript modules
│   └── synth.ts              # TypeScript modules (Bun)
└── README.md
```

### Example package.json

```json
{
  "name": "my-pd-patch",
  "version": "1.0.0",
  "dependencies": {
    "lodash": "^4.17.21",
    "tone": "^14.7.77"
  }
}
```

## 🔧 Runtime Detection

pd-node automatically detects and uses the best available runtime:

1. **TypeScript file (.ts/.tsx)?** → Requires Bun
2. **Bun available?** → Use Bun (fastest, TypeScript support)
3. **Node.js available?** → Use Node.js (compatible)
4. **None available?** → Show helpful error with install instructions

### Check Runtime

```
[node --version]  →  Prints detected runtime info to console
```

## 📖 Examples

See `examples/` directory:

- `01-npm-packages/` - Using lodash, ramda, etc.
- `02-async-fetch/` - Web APIs and async/await
- `03-websocket-server/` - Real-time communication
- `04-ai-integration/` - Using AI/ML libraries

## 🎓 Comparison: Max/MSP Migration

### Max [js] → pdjs

```javascript
// Max [js] (SpiderMonkey 2011)
var x = 10;
function double() {
    return x * 2;
}

// pdjs (Modern V8)
const x = 10;
const double = () => x * 2;
```

### Max [node.script] → pd-node

```javascript
// Max (max-api)
const maxApi = require('max-api');

maxApi.addHandler('bang', () => {
    maxApi.outlet('hello');
});

// PD (pd-api) - Nearly identical!
const pd = require('pd-api');

pd.on('bang', () => {
    pd.outlet(0, 'hello');
});
```

**Key difference:** pd-node is 3MB download, Max ships 2GB with bundled Node.js

## 🤝 Contributing

This is an open-source project. Contributions welcome!

### Development Setup

```bash
git clone https://github.com/theslyprofessor/pd-node.git
cd pd-node
mkdir build && cd build
cmake ..
make
```

### Architecture

See `.openspec/` directory for:
- Research documents
- Implementation proposals
- Max/MSP comparison analysis

## 📄 License

MIT License - See LICENSE file

## 🙏 Credits

- Inspired by Max/MSP's [js] and [node.script] objects
- Built on [pd.build](https://github.com/pierreguillot/pd.build) framework
- Sister project to [pdjs](https://github.com/mganss/pdjs)

## 🔗 Links

- [Pure Data](https://puredata.info/)
- [Bun Runtime](https://bun.sh/)
- [Node.js](https://nodejs.org/)
- [pdjs (V8 for PD)](https://github.com/mganss/pdjs)

---

**pd-node: 3MB of power, infinite possibilities.** 🚀
