// Simple test script for pd-node
const pd = require('pd-api');

console.log('Hello from pd-node!');
console.log('JavaScript is running in Bun/Node.js!');

pd.on('bang', () => {
    pd.outlet(0, 'Hello from JavaScript!');
    console.log('Sent: Hello from JavaScript!');
});

pd.on('float', (value) => {
    const doubled = value * 2;
    pd.outlet(0, doubled);
    console.log(`Received float: ${value}, sent back: ${doubled}`);
});

pd.on('list', (...args) => {
    console.log('Received list:', args);
    const sum = args.reduce((a, b) => a + b, 0);
    pd.outlet(0, sum);
    console.log(`Sum: ${sum}`);
});

console.log('pd-node handlers registered successfully!');
