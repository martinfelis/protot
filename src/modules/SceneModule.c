#include "RuntimeModule.h"
#include "RenderCommands.h"
#include "Utils.h"

typedef struct {
} module_state;

static struct module_state *module_init() {
	gLog ("%s %s called", __FILE__, __FUNCTION__);
	module_state *state = (module_state*) malloc(sizeof(*state));

	return state;
}

static void module_finalize(struct module_state *state) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
	free(state);
}

static void module_reload(struct module_state *state, void* read_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
}

static void module_unload(struct module_state *state, void* write_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
}

static bool module_step(struct module_state *state, float dt) {
	gLog ("Scene step4");
	float mat[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f};

	RenderCommandsClear();

	RenderSubmit(
			DebugSphere,
			mat,
			color
			);
	return true;
}

const struct module_api MODULE_API = {
	.init = module_init,
	.finalize = module_finalize,
	.reload = module_reload,
	.unload = module_unload,
	.step = module_step
};
