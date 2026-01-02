# 🎨 pd-node Demo Patches

**Open these patches in Pure Data to see pd-node in action!**

## Getting Started

### 1. hello-demo.pd - Your First Test
```
Click bangs → See JavaScript execute → Check console logs
```

**What it shows:**
- ✅ JavaScript receives Pure Data messages
- ✅ JavaScript sends messages back
- ✅ Console logging works
- ✅ Basic float processing (doubling numbers)

**Time:** 30 seconds  
**Difficulty:** Beginner  
**File:** `../examples/hello.js`

---

### 2. teoria-demo.pd - Real npm Packages
```
Enter note names (C4, D#4, etc.) → Get major chord MIDI notes
```

**What it shows:**
- ✅ npm package integration (teoria.js)
- ✅ Music theory calculations in JavaScript
- ✅ Symbol → List conversion
- ✅ Real-world use case

**Time:** 2 minutes  
**Difficulty:** Intermediate  
**File:** `~/pd-scripts/chords.js`  
**Setup:** Create ~/pd-scripts/ and copy examples/chords.js there

---

### 3. weather-demo.pd - Live Data Sonification
```
Bang → Fetch weather → Convert to musical parameters
```

**What it shows:**
- ✅ async/await in Pure Data!
- ✅ REST API calls (wttr.in)
- ✅ JSON parsing
- ✅ Data → sound parameter mapping
- ✅ Real-time internet data

**Time:** 5 minutes  
**Difficulty:** Advanced  
**Requirements:** Internet connection  
**File:** `~/pd-scripts/weather-sonify.js`

---

## Quick Setup

### For teoria-demo.pd and weather-demo.pd:

```bash
# Create scripts directory
mkdir -p ~/pd-scripts

# Copy examples
cp ../examples/chords.js ~/pd-scripts/
cp ../examples/weather-sonify.js ~/pd-scripts/

# Install teoria package
cd ~/pd-scripts
bun init -y
bun install teoria
```

---

## Help & Reference

- **node-help.pd** - Official help file (right-click any [node] object → Help)
- **test-patch.pd** - Developer test patch (for testing new features)

---

## Tips

1. **Open Plugdata Console** - See JavaScript console.log() output
2. **Check Paths** - All paths in patches are relative (../ means parent directory)
3. **Try TypeScript** - Rename any .js to .ts and it works with Bun!
4. **npm Packages** - Install any package with `bun install <package>` or `npm install <package>`

---

## Troubleshooting

**"No such file or directory"**
- Check that examples/ directory exists in parent folder
- Or modify patch paths to match your setup

**"Bun not found"**
```bash
curl -fsSL https://bun.sh/install | bash
```

**"Package not found"**
```bash
cd ~/pd-scripts
bun install teoria  # or npm install teoria
```

**Weather demo not working?**
- Check internet connection
- Try: `curl https://wttr.in/?format=j1` in terminal to test API

---

**Ready?** Open `hello-demo.pd` and click the bang! 🚀
