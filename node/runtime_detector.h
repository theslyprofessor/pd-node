/**
 * runtime_detector.h
 * 
 * Detects and manages JavaScript runtime selection (Bun vs Node.js)
 */

#ifndef PD_NODE_RUNTIME_DETECTOR_H
#define PD_NODE_RUNTIME_DETECTOR_H

#include <string>

namespace pdnode {

/**
 * Available JavaScript runtimes
 */
enum class Runtime {
    NONE,    // No runtime available
    BUN,     // Bun runtime (preferred)
    NODE     // Node.js runtime (fallback)
};

/**
 * Script type detection
 */
enum class ScriptType {
    JAVASCRIPT,   // .js files
    TYPESCRIPT    // .ts, .tsx files
};

/**
 * Runtime detector and selector
 */
class RuntimeDetector {
public:
    RuntimeDetector();
    
    /**
     * Get appropriate runtime for a script
     * 
     * @param script_path Path to script file
     * @return Runtime to use (or NONE if unavailable)
     */
    Runtime get_runtime_for_script(const std::string& script_path) const;
    
    /**
     * Check if a specific runtime is available
     */
    bool is_bun_available() const { return bun_available_; }
    bool is_node_available() const { return node_available_; }
    bool has_any_runtime() const { return bun_available_ || node_available_; }
    
    /**
     * Get runtime executable path
     */
    std::string get_runtime_path(Runtime runtime) const;
    
    /**
     * Get runtime version string
     */
    std::string get_runtime_version(Runtime runtime) const;
    
    /**
     * Get human-readable runtime name
     */
    std::string get_runtime_name(Runtime runtime) const;
    
    /**
     * Get helpful error message if no runtime available
     */
    std::string get_error_message(const std::string& script_path) const;
    
    /**
     * Get info string for debugging
     */
    std::string get_info_string() const;
    
private:
    void detect_runtimes();
    ScriptType detect_script_type(const std::string& path) const;
    
    bool bun_available_ = false;
    bool node_available_ = false;
    
    std::string bun_path_;
    std::string bun_version_;
    std::string node_path_;
    std::string node_version_;
};

} // namespace pdnode

#endif // PD_NODE_RUNTIME_DETECTOR_H
