/**
 * pd-api - Pure Data API for pd-node external
 * 
 * Mirrors Max/MSP's max-api design but for Pure Data
 * Provides clean interface for JavaScript/TypeScript in PD
 */

// Handler storage
const handlers = new Map();

// Current context (set during message dispatch)
let currentInlet = 0;
let currentMessage = '';

// Internal bridge injected by C++ code
// Similar to how max-api gets functions from Max
const _internal = global.__pd_internal__ || {
    // Fallback implementations for testing outside PD
    outlet: (...args) => console.log('OUTLET:', ...args),
    post: (msg) => console.log('POST:', msg),
    error: (msg) => console.error('ERROR:', msg),
    registerHandler: (name) => {},
    inlets: 1,
    outlets: 1,
    jsarguments: []
};

/**
 * Main pd-api object
 */
const pd = {
    /**
     * Properties (read-only)
     */
    get inlets() {
        return _internal.inlets || 1;
    },
    
    get outlets() {
        return _internal.outlets || 1;
    },
    
    get inlet() {
        return currentInlet;
    },
    
    get messagename() {
        return currentMessage;
    },
    
    get args() {
        return _internal.jsarguments || [];
    },
    
    /**
     * Output to outlets
     * 
     * @param {number} outlet - Outlet index (0-based)
     * @param {...any} values - Values to send
     * 
     * @example
     * pd.outlet(0);              // Send bang
     * pd.outlet(0, 42);          // Send float
     * pd.outlet(0, 'hello');     // Send symbol
     * pd.outlet(0, [1, 2, 3]);   // Send list
     */
    outlet(outlet, ...values) {
        if (values.length === 0) {
            // Bang
            _internal.outlet(outlet, 'bang');
        } else if (values.length === 1) {
            const value = values[0];
            if (typeof value === 'number') {
                _internal.outlet(outlet, 'float', value);
            } else if (typeof value === 'string') {
                _internal.outlet(outlet, 'symbol', value);
            } else {
                _internal.outlet(outlet, 'anything', value);
            }
        } else {
            // List
            _internal.outlet(outlet, 'list', ...values);
        }
    },
    
    /**
     * Print message to PD console
     * 
     * @param {...any} args - Values to print
     * 
     * @example
     * pd.post('Hello', 'world');  // "Hello world"
     */
    post(...args) {
        _internal.post(args.join(' '));
    },
    
    /**
     * Print error to PD console
     * 
     * @param {...any} args - Error message
     * 
     * @example
     * pd.error('Something went wrong!');
     */
    error(...args) {
        _internal.error(args.join(' '));
    },
    
    /**
     * Register message handler
     * 
     * @param {string} message - Message name ('bang', 'float', 'list', etc.)
     * @param {Function} callback - Handler function
     * 
     * @example
     * pd.on('bang', (inlet) => {
     *     pd.outlet(0, 'received bang');
     * });
     * 
     * pd.on('float', (inlet, value) => {
     *     pd.outlet(0, value * 2);
     * });
     * 
     * pd.on('list', (inlet, values) => {
     *     const sum = values.reduce((a, b) => a + b, 0);
     *     pd.outlet(0, sum);
     * });
     */
    on(message, callback) {
        if (typeof callback !== 'function') {
            throw new TypeError('Callback must be a function');
        }
        
        // Register with wrapper.js internal API
        if (_internal.register) {
            _internal.register(message, callback);
        } else {
            // Fallback for testing outside PD
            if (!handlers.has(message)) {
                handlers.set(message, []);
            }
            handlers.get(message).push(callback);
        }
    },
    
    /**
     * Remove message handler
     * 
     * @param {string} message - Message name
     * @param {Function} [callback] - Specific callback to remove (optional)
     */
    off(message, callback) {
        if (!handlers.has(message)) return;
        
        if (callback) {
            // Remove specific callback
            const callbacks = handlers.get(message);
            const index = callbacks.indexOf(callback);
            if (index !== -1) {
                callbacks.splice(index, 1);
            }
        } else {
            // Remove all handlers for this message
            handlers.delete(message);
        }
    },
    
    /**
     * Internal: Dispatch message from C++
     * Called by pd-node C++ code when message arrives
     * 
     * @private
     */
    _dispatch(inlet, message, ...args) {
        currentInlet = inlet;
        currentMessage = message;
        
        const messageHandlers = handlers.get(message);
        if (messageHandlers && messageHandlers.length > 0) {
            for (const handler of messageHandlers) {
                try {
                    handler(inlet, ...args);
                } catch (err) {
                    pd.error(`Handler error in '${message}': ${err.message}`);
                    if (err.stack) {
                        pd.error(err.stack);
                    }
                }
            }
        }
        
        // Reset context
        currentInlet = 0;
        currentMessage = '';
    },
    
    /**
     * Get version info
     */
    get version() {
        return '0.1.0';
    }
};

// Export for CommonJS (Node.js/Bun)
module.exports = pd;

// Export for ES modules (Bun native)
if (typeof exports !== 'undefined') {
    exports.pd = pd;
    exports.default = pd;
}
