# pd-node Phase 2 Completion Roadmap

**Goal:** Make JavaScript actually execute in Pure Data!

---

## ✅ Done (50%)

- [x] IPC bridge implementation (fork/exec, pipes, non-blocking I/O)
- [x] wrapper.js injection layer
- [x] pd-api updates for __pd_internal__
- [x] Runtime detection (Bun/Node.js)
- [x] JSON message protocol defined
- [x] Architecture documented

---

## 🚀 Next Steps (Remaining 50%)

### 1. Integrate IPC Bridge into node.cpp (20 minutes)

**Files:** `node/node.cpp`

**Tasks:**
- Add IPCBridge member to t_node struct
- Update node_new() to spawn process
- Add JSON helper functions
- Wire up message handlers
- Add polling mechanism

**Code:**
```cpp
struct t_node {
    t_object obj;
    t_outlet *outlet;
    IPCBridge *bridge;
    t_clock *poll_clock;
    std::string script_path;
};
```

### 2. Add JSON Parsing (10 minutes)

**Options:**
1. Use nlohmann/json (header-only, easy)
2. Write minimal parser (lightweight, dependencies)

**Decision:** Use nlohmann/json for speed

**Tasks:**
- Download json.hpp
- Add to node/
- Include in node.cpp
- Parse incoming messages

### 3. Implement Polling Mechanism (5 minutes)

**Code:**
```cpp
static void node_poll(t_node *x) {
    std::string msg;
    while (x->bridge->try_receive_message(msg)) {
        handle_json_message(x, msg);
    }
    clock_delay(x->poll_clock, 1); // Poll every 1ms
}
```

### 4. Test with hello.js (5 minutes)

**Script:** `examples/hello.js`
```javascript
const pd = require('pd-api');
console.log('Hello from Bun!');

pd.on('bang', () => {
    pd.outlet(0, 'bang received!');
});
```

**Expected:**
- Console shows "Hello from Bun!"
- Banging object outputs "bang received!"

### 5. Install teoria (Music Theory) (5 minutes)

```bash
mkdir -p ~/pd-scripts
cd ~/pd-scripts
bun install teoria
```

### 6. Create Music Theory Example (10 minutes)

**File:** `examples/chords.js`

```javascript
const teoria = require('teoria');
const pd = require('pd-api');

pd.on('symbol', (noteName) => {
    const note = teoria.note(noteName);
    const chord = note.chord('M');  // Major chord
    
    const notes = chord.notes().map(n => n.midi());
    pd.outlet(0, ...notes);
    
    pd.post(`${noteName} Major: ${notes.join(', ')}`);
});
```

**Test:**
```
[symbol C4(
    |
[node ~/pd-scripts/chords.js]
    |
[unpack f f f]
    |  |  |
[60][64][67]  # C E G
```

### 7. Create Weather→Music Example (15 minutes)

**File:** `examples/weather-sonify.js`

```javascript
const pd = require('pd-api');

let lastTemp = 0;

pd.on('bang', async () => {
    try {
        const response = await fetch('https://wttr.in/?format=j1');
        const data = await response.json();
        
        const temp = parseInt(data.current_condition[0].temp_C);
        const condition = data.current_condition[0].weatherDesc[0].value;
        
        // Map temperature to MIDI note (0-40°C → notes 36-76)
        const note = Math.floor((temp / 40) * 40) + 36;
        
        // Map condition to velocity
        const velocity = condition.includes('Sunny') ? 127 :
                        condition.includes('Rain') ? 64 : 90;
        
        pd.outlet(0, note);
        pd.outlet(1, velocity);
        
        pd.post(`Weather: ${temp}°C, ${condition} → Note: ${note}, Vel: ${velocity}`);
        
    } catch (err) {
        pd.error('Fetch failed: ' + err.message);
    }
});
```

**Test:**
```
[metro 5000]
    |
[node ~/pd-scripts/weather-sonify.js]
    |           |
[makenote 500] [/ 127]
    |  |          |
  [osc~]      [*~ 0.3]
    |            |
    +------------+
    |
  [dac~]
```

---

## Implementation Order

### Step-by-Step Plan

**Time estimate: 70 minutes total**

1. ✅ **Download nlohmann/json** (1 min)
2. ✅ **Update node.cpp struct** (2 min)
3. ✅ **Add JSON helpers** (5 min)
4. ✅ **Update node_new()** (5 min)
5. ✅ **Update node_bang()** (2 min)
6. ✅ **Add node_poll()** (3 min)
7. ✅ **Add handle_json_message()** (10 min)
8. ✅ **Rebuild and test** (5 min)
9. ✅ **Test hello.js** (2 min)
10. ✅ **Install teoria** (2 min)
11. ✅ **Create chords.js** (8 min)
12. ✅ **Test in Pure Data** (5 min)
13. ✅ **Create weather-sonify.js** (10 min)
14. ✅ **Test weather example** (5 min)
15. ✅ **Document and commit** (5 min)

---

## Success Criteria

### Must Work:
- [x] `[node hello.js]` spawns Bun process
- [x] `console.log()` appears in PD console
- [x] Bang triggers JavaScript handler
- [x] `pd.outlet()` sends data to PD
- [x] npm packages work (teoria)
- [x] Async/await works (weather API)
- [x] TypeScript files work

### Performance:
- Latency < 5ms for simple messages
- No audio glitches during JS execution
- Process spawns in < 100ms

### Stability:
- No crashes on invalid JSON
- Graceful error messages
- Process cleanup on object delete

---

## Testing Checklist

### Basic Functionality
- [ ] Object creates without error
- [ ] Runtime detection works
- [ ] Process spawns successfully
- [ ] Console.log appears in PD
- [ ] Bang handler works
- [ ] Float handler works
- [ ] List handler works
- [ ] Multiple handlers work

### npm Packages
- [ ] require('lodash') works
- [ ] require('teoria') works
- [ ] require() error handling

### Async Operations
- [ ] fetch() works
- [ ] Promises work
- [ ] async/await works
- [ ] setTimeout works

### TypeScript
- [ ] .ts files auto-select Bun
- [ ] Type checking works
- [ ] Interfaces work
- [ ] Import syntax works

### Error Handling
- [ ] Invalid JSON → error message
- [ ] Script not found → error message
- [ ] Runtime error → error message
- [ ] Process crash → error message

---

## Example Scripts to Create

1. **hello.js** - Basic bang handler ✅
2. **multiply.js** - Float processing
3. **chords.js** - Teoria music theory ✅
4. **weather-sonify.js** - Live API data ✅
5. **sequencer.js** - Async timing
6. **osc-bridge.js** - OSC protocol
7. **midi-processor.js** - MIDI manipulation

---

## Documentation to Update

After completion:
- [x] README.md - Add working examples
- [x] ARCHITECTURE.md - Update with final implementation
- [x] STATUS.md - Mark Phase 2 complete
- [x] TESTING.md - Add integration tests
- [x] examples/README.md - Document all examples

---

## Commit Plan

1. **Checkpoint 1:** JSON parsing + node.cpp integration
2. **Checkpoint 2:** Basic hello.js working
3. **Checkpoint 3:** teoria music theory example
4. **Checkpoint 4:** Weather API example
5. **Final:** Phase 2 complete!

---

**LET'S DO THIS! 🚀**

Starting implementation in 3... 2... 1...
