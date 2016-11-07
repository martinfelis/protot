#pragma once

#include <stdbool.h>

struct module_state;

struct module_api {
    /**
     * @return a fresh module state
     */
    struct module_state *(*init)();

    /**
     * Destroys a module state.
     */
    void (*finalize)(struct module_state *state);

    /**
     * Called exactly once when the module code is reloaded.
     */
    void (*reload)(struct module_state *state);

    /**
     * Called exactly once when the module code is about to be reloaded.
     */
    void (*unload)(struct module_state *state);

    /**
     * Called at a regular interval by the main program.
     * @return true if the program should continue
     */
    bool (*step)(struct module_state *state, float dt);
};

extern "C" {
extern const struct module_api MODULE_API;
}
