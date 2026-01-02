/**
 * ipc_bridge.cpp
 * 
 * IPC bridge for communicating with Bun/Node.js process
 * Phase 2: Process spawning and stdin/stdout communication
 */

#include "ipc_bridge.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <iostream>

namespace pdnode {

IPCBridge::IPCBridge(const std::string& runtime_path, const std::string& wrapper_path, const std::string& script_path)
    : runtime_path_(runtime_path)
    , wrapper_path_(wrapper_path)
    , script_path_(script_path)
    , child_pid_(-1)
{
    stdin_pipe_[0] = stdin_pipe_[1] = -1;
    stdout_pipe_[0] = stdout_pipe_[1] = -1;
    stderr_pipe_[0] = stderr_pipe_[1] = -1;
}

IPCBridge::~IPCBridge() {
    terminate();
}

bool IPCBridge::spawn() {
    // Create pipes for stdin, stdout, stderr
    if (pipe(stdin_pipe_) < 0) {
        std::cerr << "[node] Failed to create stdin pipe" << std::endl;
        return false;
    }
    if (pipe(stdout_pipe_) < 0) {
        std::cerr << "[node] Failed to create stdout pipe" << std::endl;
        close(stdin_pipe_[0]);
        close(stdin_pipe_[1]);
        return false;
    }
    if (pipe(stderr_pipe_) < 0) {
        std::cerr << "[node] Failed to create stderr pipe" << std::endl;
        close(stdin_pipe_[0]);
        close(stdin_pipe_[1]);
        close(stdout_pipe_[0]);
        close(stdout_pipe_[1]);
        return false;
    }
    
    // Fork the process
    child_pid_ = fork();
    
    if (child_pid_ < 0) {
        // Fork failed
        std::cerr << "[node] Fork failed" << std::endl;
        return false;
    }
    
    if (child_pid_ == 0) {
        // Child process
        
        // Redirect stdin
        close(stdin_pipe_[1]);  // Close write end
        dup2(stdin_pipe_[0], STDIN_FILENO);
        close(stdin_pipe_[0]);
        
        // Redirect stdout
        close(stdout_pipe_[0]);  // Close read end
        dup2(stdout_pipe_[1], STDOUT_FILENO);
        close(stdout_pipe_[1]);
        
        // Redirect stderr
        close(stderr_pipe_[0]);  // Close read end
        dup2(stderr_pipe_[1], STDERR_FILENO);
        close(stderr_pipe_[1]);
        
        // Execute the runtime with wrapper.js and user script
        // Command: bun wrapper.js user_script.js
        execl(runtime_path_.c_str(), 
              runtime_path_.c_str(),
              wrapper_path_.c_str(),
              script_path_.c_str(),
              nullptr);
        
        // If we get here, exec failed
        std::cerr << "[node] exec failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Parent process
    
    // Close unused pipe ends
    close(stdin_pipe_[0]);   // We don't read from stdin
    close(stdout_pipe_[1]);  // We don't write to stdout
    close(stderr_pipe_[1]);  // We don't write to stderr
    
    // Set stdout and stderr to non-blocking
    set_nonblocking(stdout_pipe_[0]);
    set_nonblocking(stderr_pipe_[0]);
    
    return true;
}

bool IPCBridge::is_running() const {
    if (child_pid_ <= 0) {
        return false;
    }
    
    // Check if process is still alive
    int status;
    pid_t result = waitpid(child_pid_, &status, WNOHANG);
    
    if (result == 0) {
        // Process is still running
        return true;
    } else {
        // Process has exited
        return false;
    }
}

void IPCBridge::send_message(const std::string& json_message) {
    if (stdin_pipe_[1] < 0) {
        return;
    }
    
    // Send message with newline delimiter
    std::string msg = json_message + "\n";
    write(stdin_pipe_[1], msg.c_str(), msg.length());
}

bool IPCBridge::try_receive_message(std::string& out_message) {
    if (stdout_pipe_[0] < 0) {
        return false;
    }
    
    // Read available data (non-blocking)
    char buffer[4096];
    ssize_t n = read(stdout_pipe_[0], buffer, sizeof(buffer) - 1);
    
    if (n <= 0) {
        return false;  // No data available or error
    }
    
    // Add to read buffer
    buffer[n] = '\0';
    read_buffer_ += buffer;
    
    // Check if we have a complete line (delimited by \n)
    size_t newline_pos = read_buffer_.find('\n');
    if (newline_pos == std::string::npos) {
        return false;  // No complete line yet
    }
    
    // Extract the line
    out_message = read_buffer_.substr(0, newline_pos);
    read_buffer_ = read_buffer_.substr(newline_pos + 1);
    
    return true;
}

void IPCBridge::on_message(std::function<void(const std::string&)> callback) {
    message_callback_ = callback;
}

void IPCBridge::terminate() {
    if (child_pid_ > 0) {
        // Send SIGTERM
        kill(child_pid_, SIGTERM);
        
        // Wait briefly for graceful shutdown
        usleep(100000);  // 100ms
        
        // Check if still running
        if (is_running()) {
            // Force kill
            kill(child_pid_, SIGKILL);
        }
        
        // Wait for process to clean up
        waitpid(child_pid_, nullptr, 0);
        
        child_pid_ = -1;
    }
    
    // Close pipes
    if (stdin_pipe_[1] >= 0) {
        close(stdin_pipe_[1]);
        stdin_pipe_[1] = -1;
    }
    if (stdout_pipe_[0] >= 0) {
        close(stdout_pipe_[0]);
        stdout_pipe_[0] = -1;
    }
    if (stderr_pipe_[0] >= 0) {
        close(stderr_pipe_[0]);
        stderr_pipe_[0] = -1;
    }
}

void IPCBridge::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace pdnode
