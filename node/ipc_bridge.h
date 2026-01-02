/**
 * ipc_bridge.h
 * 
 * IPC bridge for communicating with Bun/Node.js runtime process
 * Phase 2: Process spawning and stdin/stdout communication
 */

#ifndef PD_NODE_IPC_BRIDGE_H
#define PD_NODE_IPC_BRIDGE_H

#include <string>
#include <functional>
#include <unistd.h>

namespace pdnode {

/**
 * IPC Bridge - Spawns and communicates with Bun/Node.js process
 */
class IPCBridge {
public:
    IPCBridge(const std::string& runtime_path, const std::string& script_path);
    ~IPCBridge();
    
    /**
     * Spawn the Bun/Node.js process
     * Returns true if successful
     */
    bool spawn();
    
    /**
     * Check if process is running
     */
    bool is_running() const;
    
    /**
     * Send a message to the JavaScript process (via stdin)
     */
    void send_message(const std::string& json_message);
    
    /**
     * Try to read a message from JavaScript (via stdout)
     * Non-blocking. Returns true if message was read.
     */
    bool try_receive_message(std::string& out_message);
    
    /**
     * Set callback for when we receive stdout from JS
     */
    void on_message(std::function<void(const std::string&)> callback);
    
    /**
     * Terminate the child process
     */
    void terminate();
    
private:
    std::string runtime_path_;
    std::string script_path_;
    
    pid_t child_pid_;
    
    int stdin_pipe_[2];   // We write to [1], child reads from [0]
    int stdout_pipe_[2];  // Child writes to [1], we read from [0]
    int stderr_pipe_[2];  // Child writes to [1], we read from [0]
    
    std::function<void(const std::string&)> message_callback_;
    
    // Buffer for reading lines
    std::string read_buffer_;
    
    void set_nonblocking(int fd);
    std::string read_line_nonblocking(int fd);
};

} // namespace pdnode

#endif // PD_NODE_IPC_BRIDGE_H
