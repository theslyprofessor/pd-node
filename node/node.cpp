/**
 * node.cpp
 * 
 * pd-node - Modern JavaScript & TypeScript for Pure Data
 * Main external implementation
 */

#include <m_pd.h>
#include <g_canvas.h>
#include "runtime_detector.h"
#include <string>
#include <vector>

using namespace pdnode;

static t_class *node_class;

typedef struct _node {
    t_object x_obj;
    t_canvas *canvas;
    
    std::string script_path;
    Runtime runtime;
    RuntimeDetector* detector;
    
    // TODO: IPC bridge for communicating with Bun/Node process
    // TODO: Inlets/outlets management
} t_node;

// Forward declarations
static void *node_new(t_symbol *s, int argc, t_atom *argv);
static void node_free(t_node *x);
static void node_bang(t_node *x);
static void node_float(t_node *x, t_float f);
static void node_anything(t_node *x, t_symbol *s, int argc, t_atom *argv);

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
    
    class_addbang(node_class, node_bang);
    class_addfloat(node_class, node_float);
    class_addanything(node_class, node_anything);
    
    post("pd-node v" PD_NODE_VERSION " - Modern JavaScript & TypeScript for Pure Data");
    post("  Repository: https://github.com/theslyprofessor/pd-node");
}

/**
 * Create new [node] object
 */
static void *node_new(t_symbol *s, int argc, t_atom *argv) {
    t_node *x = (t_node *)pd_new(node_class);
    x->canvas = canvas_getcurrent();
    x->detector = new RuntimeDetector();
    
    // Parse arguments: [node script.js] or [node --bun script.js] etc.
    if (argc < 1) {
        pd_error(x, "[node]: requires script path argument");
        pd_error(x, "  Usage: [node script.js] or [node script.ts]");
        return x;
    }
    
    // Get script path (first non-flag argument)
    const char* script_name = nullptr;
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            const char* arg = atom_getsymbol(&argv[i])->s_name;
            if (arg[0] != '-') {  // Not a flag
                script_name = arg;
                break;
            }
        }
    }
    
    if (!script_name) {
        pd_error(x, "[node]: no script path provided");
        return x;
    }
    
    // Resolve full path relative to patch directory
    const char* canvas_dir = canvas_getdir(x->canvas)->s_name;
    x->script_path = std::string(canvas_dir) + "/" + script_name;
    
    // Detect runtime
    x->runtime = x->detector->get_runtime_for_script(x->script_path);
    
    if (x->runtime == Runtime::NONE) {
        // No runtime available
        pd_error(x, "[node]: %s", x->detector->get_error_message(x->script_path).c_str());
        return x;
    }
    
    // Print runtime info
    post("[node] Loading %s", script_name);
    post("[node] Runtime: %s %s", 
         x->detector->get_runtime_name(x->runtime).c_str(),
         x->detector->get_runtime_version(x->runtime).c_str());
    
    // TODO: Initialize IPC bridge and spawn runtime process
    // TODO: Load script and initialize pd-api
    
    // Create one outlet for now
    outlet_new(&x->x_obj, &s_anything);
    
    return x;
}

/**
 * Free [node] object
 */
static void node_free(t_node *x) {
    // TODO: Terminate runtime process
    if (x->detector) {
        delete x->detector;
    }
}

/**
 * Handle bang message
 */
static void node_bang(t_node *x) {
    // TODO: Send bang to JavaScript via IPC
    post("[node] Received bang (not yet implemented)");
}

/**
 * Handle float message
 */
static void node_float(t_node *x, t_float f) {
    // TODO: Send float to JavaScript via IPC
    post("[node] Received float: %f (not yet implemented)", f);
}

/**
 * Handle any other message
 */
static void node_anything(t_node *x, t_symbol *s, int argc, t_atom *argv) {
    // TODO: Send message to JavaScript via IPC
    post("[node] Received %s with %d arguments (not yet implemented)", 
         s->s_name, argc);
}
