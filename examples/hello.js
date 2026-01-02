// Simple test script for pd-node
const pd = require('pd-api');

console.log('hello.js loaded successfully!');

pd.on('bang', () => {
    pd.outlet(0, 'hello from JavaScript!');
    console.log('Sent hello message');
});

pd.on('float', (value) => {
    pd.outlet(0, 'float', value * 2);
    console.log('Received float:', value, 'sent back:', value * 2);
});

pd.on('list', (...args) => {
    console.log('Received list:', args);
    pd.outlet(0, 'list', ...args.reverse());
});
