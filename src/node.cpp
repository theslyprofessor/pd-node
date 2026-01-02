/**
 * node.cpp
 * 
 * pd-node - Modern JavaScript & TypeScript for Pure Data
 * Phase 2: Full IPC bridge integration
 */

#include <m_pd.h>
#include <g_canvas.h>
#include "runtime_detector.h"
#include "ipc_bridge.h"
#include "json.hpp"
#include <string>
#include <vector>

using namespace pdnode;
using json = nlohmann::json;

static t_class *node_class;

typedef struct _node {
    t_object x_obj;
    t_canvas *canvas;
    t_outlet *outlet;
    t_clock *poll_clock;
    
    std::string script_path;
    Runtime runtime;
    RuntimeDetector* detector;
    IPCBridge* bridge;
    
    bool ready;  // True after receiving 'ready' message from JS
} t_node;

// Forward declarations
static void *node_new(t_symbol *s, int argc, t_atom *argv);
static void node_free(t_node *x);
static void node_bang(t_node *x);
static void node_float(t_node *x, t_float f);
static void node_symbol(t_node *x, t_symbol *s);
static void node_list(t_node *x, t_symbol *s, int argc, t_atom *argv);
static void node_anything(t_node *x, t_symbol *s, int argc, t_atom *argv);
static void node_poll(t_node *x);
static void handle_json_message(t_node *x, const std::string& json_str);

/**
 * External setup - called when PD loads the external
 */
extern "C" void node_setup(void) {
    node_class = class_new(
        gensym("node"),
        (t_newmethod)node_new,
        (t_method)node_free,
        sizeof(t_node),
        CLASS_DEFAULT,
        A_GIMME,  // Variable arguments
        0
    );
    
    // Register message handlers
    class_addbang(node_class, node_bang);
    class_addfloat(node_class, node_float);
    class_addsymbol(node_class, node_symbol);
    class_addlist(node_class, node_list);
    class_addanything(node_class, node_anything);
    
    post("[node] pd-node v0.1.0 - Modern JavaScript & TypeScript for Pure Data");
}

/**
 * Create new [node] object
 */
static void *node_new(t_symbol *s, int argc, t_atom *argv) {
    t_node *x = (t_node *)pd_new(node_class);
    
    // Get canvas for relative path resolution
    x->canvas = canvas_getcurrent();
    x->ready = false;
    
    // Check if script argument provided
    if (argc < 1 || argv[0].a_type != A_SYMBOL) {
        pd_error(x, "[node] requires script path as argument");
        pd_error(x, "[node] usage: [node script.js]");
        return x;
    }
    
    // Get script path
    x->script_path = atom_getsymbol(&argv[0])->s_name;
    
    // Make path absolute if needed
    if (x->script_path[0] != '/' && x->script_path[0] != '~') {
        // Relative path - resolve relative to patch directory
        char dir[MAXPDSTRING];
        char *filename;
        canvas_makefilename(x->canvas, x->script_path.c_str(), dir, MAXPDSTRING);
        x->script_path = dir;
    }
    
    // Expand ~ in path
    if (x->script_path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            x->script_path = std::string(home) + x->script_path.substr(1);
        }
    }
    
    // Create runtime detector
    x->detector = new RuntimeDetector();
    
    // Detect appropriate runtime for this script
    x->runtime = x->detector->get_runtime_for_script(x->script_path);
    
    if (x->runtime == Runtime::NONE) {
        std::string error_msg = x->detector->get_error_message(x->script_path);
        pd_error(x, "%s", error_msg.c_str());
        return x;
    }
    
    // Get runtime path
    std::string runtime_path = x->detector->get_runtime_path(x->runtime);
    std::string runtime_name = x->detector->get_runtime_name(x->runtime);
    
    post("[node] Using %s runtime: %s", runtime_name.c_str(), runtime_path.c_str());
    post("[node] Script: %s", x->script_path.c_str());
    
    // Get wrapper.js path (next to the external)
    char wrapper_path[MAXPDSTRING];
    const char *ext_path = class_gethelpdir(node_class);
    snprintf(wrapper_path, MAXPDSTRING, "%s/wrapper.js", ext_path);
    
    // Create IPC bridge
    x->bridge = new IPCBridge(runtime_path, std::string(wrapper_path), x->script_path);
    
    // Spawn the process
    if (!x->bridge->spawn()) {
        pd_error(x, "[node] Failed to spawn %s process", runtime_name.c_str());
        delete x->bridge;
        x->bridge = nullptr;
        return x;
    }
    
    post("[node] Process spawned successfully");
    
    // Create outlet
    x->outlet = outlet_new(&x->x_obj, &s_anything);
    
    // Set up polling clock (poll every 1ms)
    x->poll_clock = clock_new(x, (t_method)node_poll);
    clock_delay(x->poll_clock, 1);
    
    return x;
}

/**
 * Free [node] object
 */
static void node_free(t_node *x) {
    if (x->poll_clock) {
        clock_free(x->poll_clock);
    }
    
    if (x->bridge) {
        x->bridge->terminate();
        delete x->bridge;
    }
    
    if (x->detector) {
        delete x->detector;
    }
}

/**
 * Handle bang message
 */
static void node_bang(t_node *x) {
    if (!x->bridge || !x->ready) {
        return;
    }
    
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", "bang"},
        {"args", json::array()}
    };
    
    x->bridge->send_message(msg.dump());
}

/**
 * Handle float message
 */
static void node_float(t_node *x, t_float f) {
    if (!x->bridge || !x->ready) {
        return;
    }
    
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", "float"},
        {"args", json::array({f})}
    };
    
    x->bridge->send_message(msg.dump());
}

/**
 * Handle symbol message
 */
static void node_symbol(t_node *x, t_symbol *s) {
    if (!x->bridge || !x->ready) {
        return;
    }
    
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", "symbol"},
        {"args", json::array({s->s_name})}
    };
    
    x->bridge->send_message(msg.dump());
}

/**
 * Handle list message
 */
static void node_list(t_node *x, t_symbol *s, int argc, t_atom *argv) {
    if (!x->bridge || !x->ready) {
        return;
    }
    
    json args = json::array();
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_FLOAT) {
            args.push_back(atom_getfloat(&argv[i]));
        } else if (argv[i].a_type == A_SYMBOL) {
            args.push_back(atom_getsymbol(&argv[i])->s_name);
        }
    }
    
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", "list"},
        {"args", args}
    };
    
    x->bridge->send_message(msg.dump());
}

/**
 * Handle anything message (catch-all)
 */
static void node_anything(t_node *x, t_symbol *s, int argc, t_atom *argv) {
    if (!x->bridge || !x->ready) {
        return;
    }
    
    json args = json::array();
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_FLOAT) {
            args.push_back(atom_getfloat(&argv[i]));
        } else if (argv[i].a_type == A_SYMBOL) {
            args.push_back(atom_getsymbol(&argv[i])->s_name);
        }
    }
    
    json msg = {
        {"type", "message"},
        {"inlet", 0},
        {"selector", s->s_name},
        {"args", args}
    };
    
    x->bridge->send_message(msg.dump());
}

/**
 * Poll for messages from JavaScript
 */
static void node_poll(t_node *x) {
    if (!x->bridge) {
        return;
    }
    
    // Check if process is still running
    if (!x->bridge->is_running()) {
        pd_error(x, "[node] Process terminated unexpectedly");
        delete x->bridge;
        x->bridge = nullptr;
        return;
    }
    
    // Read all available messages
    std::string msg;
    while (x->bridge->try_receive_message(msg)) {
        handle_json_message(x, msg);
    }
    
    // Schedule next poll
    clock_delay(x->poll_clock, 1);
}

/**
 * Handle JSON message from JavaScript
 */
static void handle_json_message(t_node *x, const std::string& json_str) {
    try {
        json msg = json::parse(json_str);
        
        std::string type = msg["type"];
        
        if (type == "ready") {
            x->ready = true;
            post("[node] JavaScript runtime ready");
            
        } else if (type == "outlet") {
            int outlet_num = msg["outlet"];
            std::string selector = msg["selector"];
            
            if (selector == "bang") {
                outlet_bang(x->outlet);
                
            } else if (selector == "float") {
                t_float f = msg["args"][0];
                outlet_float(x->outlet, f);
                
            } else if (selector == "symbol") {
                std::string str = msg["args"][0];
                outlet_symbol(x->outlet, gensym(str.c_str()));
                
            } else if (selector == "list") {
                json args = msg["args"];
                int argc = args.size();
                t_atom argv[argc];
                
                for (int i = 0; i < argc; i++) {
                    if (args[i].is_number()) {
                        SETFLOAT(&argv[i], args[i]);
                    } else if (args[i].is_string()) {
                        std::string str = args[i];
                        SETSYMBOL(&argv[i], gensym(str.c_str()));
                    }
                }
                
                outlet_list(x->outlet, &s_list, argc, argv);
            }
            
        } else if (type == "log") {
            std::string message = msg["message"];
            post("[node] %s", message.c_str());
            
        } else if (type == "error") {
            std::string message = msg["message"];
            pd_error(x, "[node] %s", message.c_str());
        }
        
    } catch (json::exception& e) {
        pd_error(x, "[node] JSON parse error: %s", e.what());
    }
}
