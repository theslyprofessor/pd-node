/**
 * wrapper.js
 * 
 * This script runs BEFORE the user's script.
 * It sets up the pd-api environment and message handling.
 */

// Create the internal API that pd-api will use
global.__pd_internal__ = {
    handlers: {
        bang: [],
        float: [],
        symbol: [],
        list: [],
        anything: []
    },
    
    // Called by pd-api when user registers a handler
    register: function(selector, handler) {
        if (!this.handlers[selector]) {
            this.handlers[selector] = [];
        }
        this.handlers[selector].push(handler);
    },
    
    // Called when we receive a message from C++
    dispatch: function(msg) {
        const selector = msg.selector || 'anything';
        const handlers = this.handlers[selector] || [];
        
        for (const handler of handlers) {
            try {
                handler.apply(null, msg.args || []);
            } catch (err) {
                this.error('Handler error: ' + err.message);
            }
        }
    },
    
    // Send message to PD outlet
    outlet: function(outlet, selector, ...args) {
        const msg = {
            type: 'outlet',
            outlet: outlet,
            selector: selector,
            args: args
        };
        process.stdout.write(JSON.stringify(msg) + '\n');
    },
    
    // Log message to PD console
    post: function(message) {
        const msg = {
            type: 'log',
            message: String(message)
        };
        process.stdout.write(JSON.stringify(msg) + '\n');
    },
    
    // Error message to PD console
    error: function(message) {
        const msg = {
            type: 'error',
            message: String(message)
        };
        process.stdout.write(JSON.stringify(msg) + '\n');
    }
};

// Override console.log to route through PD
console.log = function(...args) {
    global.__pd_internal__.post(args.join(' '));
};

console.error = function(...args) {
    global.__pd_internal__.error(args.join(' '));
};

// Handle messages from C++ (via stdin)
let stdinBuffer = '';

process.stdin.on('data', (data) => {
    stdinBuffer += data.toString();
    
    // Process complete lines
    let newlineIndex;
    while ((newlineIndex = stdinBuffer.indexOf('\n')) !== -1) {
        const line = stdinBuffer.substring(0, newlineIndex);
        stdinBuffer = stdinBuffer.substring(newlineIndex + 1);
        
        if (line.trim()) {
            try {
                const msg = JSON.parse(line);
                
                if (msg.type === 'message') {
                    // Dispatch to user's handlers
                    global.__pd_internal__.dispatch(msg);
                }
            } catch (err) {
                global.__pd_internal__.error('Parse error: ' + err.message);
            }
        }
    }
});

// Signal that we're ready
process.stdout.write(JSON.stringify({ type: 'ready' }) + '\n');

// Now load the user's script (passed as first argument)
const userScript = process.argv[2];
if (userScript) {
    try {
        require(userScript);
    } catch (err) {
        global.__pd_internal__.error('Failed to load script: ' + err.message);
        process.exit(1);
    }
} else {
    global.__pd_internal__.error('No script specified');
    process.exit(1);
}
