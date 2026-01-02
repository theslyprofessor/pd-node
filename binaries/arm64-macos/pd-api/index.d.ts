/**
 * TypeScript definitions for pd-api
 * 
 * Pure Data API for pd-node external
 */

/**
 * Message handler callback type
 */
export type MessageHandler = (inlet: number, ...args: any[]) => void;

/**
 * Pure Data API interface
 */
export interface PD {
    /**
     * Number of inlets (read-only)
     */
    readonly inlets: number;
    
    /**
     * Number of outlets (read-only)
     */
    readonly outlets: number;
    
    /**
     * Current inlet number (during handler execution)
     */
    readonly inlet: number;
    
    /**
     * Current message name (during handler execution)
     */
    readonly messagename: string;
    
    /**
     * Arguments passed to [node] object
     */
    readonly args: any[];
    
    /**
     * API version
     */
    readonly version: string;
    
    /**
     * Send data to an outlet
     * 
     * @param outlet - Outlet index (0-based)
     * @param values - Values to send (bang if omitted, float/symbol/list)
     * 
     * @example
     * pd.outlet(0);              // Send bang
     * pd.outlet(0, 42);          // Send float
     * pd.outlet(0, 'hello');     // Send symbol
     * pd.outlet(0, [1, 2, 3]);   // Send list
     */
    outlet(outlet: number, ...values: any[]): void;
    
    /**
     * Print message to PD console
     * 
     * @param args - Values to print
     * 
     * @example
     * pd.post('Hello', 'world');
     */
    post(...args: any[]): void;
    
    /**
     * Print error to PD console
     * 
     * @param args - Error message
     * 
     * @example
     * pd.error('Something went wrong!');
     */
    error(...args: any[]): void;
    
    /**
     * Register message handler
     * 
     * @param message - Message name ('bang', 'float', 'list', 'symbol', etc.)
     * @param callback - Handler function
     * 
     * @example
     * pd.on('bang', (inlet) => {
     *     pd.outlet(0, 'received bang');
     * });
     * 
     * pd.on('float', (inlet, value: number) => {
     *     pd.outlet(0, value * 2);
     * });
     * 
     * pd.on('list', (inlet, values: number[]) => {
     *     const sum = values.reduce((a, b) => a + b, 0);
     *     pd.outlet(0, sum);
     * });
     */
    on(message: string, callback: MessageHandler): void;
    
    /**
     * Remove message handler
     * 
     * @param message - Message name
     * @param callback - Specific callback to remove (optional, removes all if omitted)
     */
    off(message: string, callback?: MessageHandler): void;
    
    /**
     * Internal dispatch (called by C++ code)
     * @private
     */
    _dispatch(inlet: number, message: string, ...args: any[]): void;
}

/**
 * Main pd-api export
 */
declare const pd: PD;

export default pd;
export { pd };
