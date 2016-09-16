#include "RuntimeModule.h"
#include "Renderer.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"

#include <iostream>
#include <sstream>

struct module_state {
	int width, height;
	int select;
	char cells[];
};

static struct module_state *module_init() {
	std::cout << "Module init called" << std::endl;
	module_state *state = (module_state*) malloc(sizeof(*state));
	return state;
}

static void module_finalize(struct module_state *state) {
	std::cout << "Module finalize called" << std::endl;
	free(state);
}

static void module_reload(struct module_state *state) {
	std::cout << "Module reload called" << std::endl;
}

static void module_unload(struct module_state *state) {
	std::cout << "Module unload called" << std::endl;
}

static bool module_step(struct module_state *state) {
	bool enabled = true;
	ImGui::Begin("yoyoyo");
	if (ImGui::Button("Baem Yahoo")) {
		std::cout << "Clicked on Baem!" << std::endl;
	}
	ImGui::End();

	float deltaTime = 0.3;
	std::ostringstream s;
	s << "TestModule:  2 Runtime Object 4 " << deltaTime << " update called!";

	bgfx::dbgTextPrintf(1, 20, 0x6f, s.str().c_str());

	return true;
}

extern "C" {

const struct module_api MODULE_API = {
	.init = module_init,
	.reload = module_reload,
	.step = module_step,
	.unload = module_unload,
	.finalize = module_finalize
};
}
