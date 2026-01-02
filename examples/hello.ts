// TypeScript test script for pd-node
import * as pd from 'pd-api';

console.log('hello.ts loaded successfully! TypeScript works!');

interface Point {
    x: number;
    y: number;
}

let count: number = 0;

pd.on('bang', () => {
    count++;
    pd.outlet(0, 'TypeScript bang #' + count);
    console.log('TypeScript bang count:', count);
});

pd.on('list', (...args: any[]) => {
    const point: Point = { x: args[0] || 0, y: args[1] || 0 };
    console.log('TypeScript received point:', point);
    pd.outlet(0, 'point', point.x, point.y);
});
