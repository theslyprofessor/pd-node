/**
 * runtime_detector.cpp
 * 
 * Detects Bun and Node.js runtimes on the system
 * Auto-selects best runtime based on script type and availability
 */

#include "runtime_detector.h"
#include <cstdlib>
#include <cstring>
#include <array>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

namespace pdnode {

// Execute command and capture output
static std::string exec_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    pclose(pipe);
    
    // Trim newline
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    return result;
}

// Check if command exists in PATH
static bool command_exists(const char* cmd) {
#ifdef _WIN32
    std::string check = std::string("where ") + cmd + " >nul 2>&1";
#else
    std::string check = std::string("which ") + cmd + " >/dev/null 2>&1";
#endif
    
    return system(check.c_str()) == 0;
}

RuntimeDetector::RuntimeDetector() {
    detect_runtimes();
}

void RuntimeDetector::detect_runtimes() {
    // Check for Bun
    if (command_exists("bun")) {
        bun_available_ = true;
        bun_path_ = exec_command("which bun");
        bun_version_ = exec_command("bun --version");
    }
    
    // Check for Node.js
    if (command_exists("node")) {
        node_available_ = true;
        node_path_ = exec_command("which node");
        node_version_ = exec_command("node --version");
    }
}

Runtime RuntimeDetector::get_runtime_for_script(const std::string& script_path) const {
    ScriptType type = detect_script_type(script_path);
    
    // TypeScript REQUIRES Bun (for now)
    if (type == ScriptType::TYPESCRIPT) {
        if (bun_available_) {
            return Runtime::BUN;
        } else {
            return Runtime::NONE;  // Will trigger error
        }
    }
    
    // JavaScript - prefer Bun, fallback to Node
    if (bun_available_) {
        return Runtime::BUN;
    } else if (node_available_) {
        return Runtime::NODE;
    }
    
    return Runtime::NONE;
}

ScriptType RuntimeDetector::detect_script_type(const std::string& path) const {
    // Check file extension
    if (path.length() >= 3) {
        std::string ext = path.substr(path.length() - 3);
        if (ext == ".ts" || (path.length() >= 4 && path.substr(path.length() - 4) == ".tsx")) {
            return ScriptType::TYPESCRIPT;
        }
    }
    
    // TODO: Could also peek at file contents for "import type", etc.
    
    return ScriptType::JAVASCRIPT;
}

std::string RuntimeDetector::get_runtime_path(Runtime runtime) const {
    switch (runtime) {
        case Runtime::BUN:
            return bun_path_;
        case Runtime::NODE:
            return node_path_;
        default:
            return "";
    }
}

std::string RuntimeDetector::get_runtime_version(Runtime runtime) const {
    switch (runtime) {
        case Runtime::BUN:
            return bun_version_;
        case Runtime::NODE:
            return node_version_;
        default:
            return "";
    }
}

std::string RuntimeDetector::get_runtime_name(Runtime runtime) const {
    switch (runtime) {
        case Runtime::BUN:
            return "Bun";
        case Runtime::NODE:
            return "Node.js";
        case Runtime::NONE:
            return "None";
    }
    return "Unknown";
}

std::string RuntimeDetector::get_error_message(const std::string& script_path) const {
    ScriptType type = detect_script_type(script_path);
    std::ostringstream oss;
    
    if (type == ScriptType::TYPESCRIPT) {
        oss << "TypeScript files require Bun runtime.\n"
            << "Install Bun: https://bun.sh\n"
            << "  curl -fsSL https://bun.sh/install | bash\n"
            << "\nAlternatively, transpile to JavaScript first.";
    } else {
        oss << "No JavaScript runtime found.\n"
            << "Install one of the following:\n"
            << "\nBun (recommended - fast, TypeScript support):\n"
            << "  https://bun.sh\n"
            << "  curl -fsSL https://bun.sh/install | bash\n"
            << "\nNode.js (compatible):\n"
            << "  https://nodejs.org\n"
            << "  brew install node (macOS)\n";
    }
    
    return oss.str();
}

std::string RuntimeDetector::get_info_string() const {
    std::ostringstream oss;
    
    oss << "pd-node runtime detection:\n";
    
    if (bun_available_) {
        oss << "  Bun: " << bun_version_ << " (" << bun_path_ << ")\n";
    } else {
        oss << "  Bun: not found\n";
    }
    
    if (node_available_) {
        oss << "  Node.js: " << node_version_ << " (" << node_path_ << ")\n";
    } else {
        oss << "  Node.js: not found\n";
    }
    
    if (!bun_available_ && !node_available_) {
        oss << "\nNo runtime available. Install Bun or Node.js.\n";
    }
    
    return oss.str();
}

} // namespace pdnode
