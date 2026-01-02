# pd-api

Pure Data API for the `pd-node` external.

Mirrors Max/MSP's `max-api` design but for Pure Data with modern JavaScript/TypeScript support.

## Installation

The `pd-api` package is automatically available when using `pd-node`. No separate installation needed!

## Usage

### JavaScript

```javascript
const pd = require('pd-api');

pd.on('bang', () => {
    pd.outlet(0, 'Hello from PD!');
});
```

### TypeScript (with Bun)

```typescript
import { pd } from 'pd-api';

pd.on('float', (inlet: number, value: number) => {
    pd.outlet(0, value * 2);
});
```

## API Reference

### Properties

- `pd.inlets` - Number of inlets (read-only)
- `pd.outlets` - Number of outlets (read-only)
- `pd.inlet` - Current inlet number (during handler)
- `pd.messagename` - Current message name (during handler)
- `pd.args` - Arguments passed to `[node]` object

### Methods

#### `pd.outlet(outlet, ...values)`

Send data to an outlet.

```javascript
pd.outlet(0);              // Bang
pd.outlet(0, 42);          // Float
pd.outlet(0, 'hello');     // Symbol
pd.outlet(0, [1, 2, 3]);   // List
```

#### `pd.on(message, callback)`

Register message handler.

```javascript
pd.on('bang', (inlet) => { /* ... */ });
pd.on('float', (inlet, value) => { /* ... */ });
pd.on('list', (inlet, values) => { /* ... */ });
pd.on('symbol', (inlet, sym) => { /* ... */ });
```

#### `pd.post(...args)`

Print to PD console.

```javascript
pd.post('Hello', 'world');  // "Hello world" in console
```

#### `pd.error(...args)`

Print error to PD console.

```javascript
pd.error('Oops!');
```

## Comparison with max-api

### Max (max-api)

```javascript
const maxApi = require('max-api');

maxApi.addHandler('bang', () => {
    maxApi.outlet('hello');
});
```

### PD (pd-api)

```javascript
const pd = require('pd-api');

pd.on('bang', () => {
    pd.outlet(0, 'hello');
});
```

Almost identical API! Migration from Max is straightforward.

## License

MIT
