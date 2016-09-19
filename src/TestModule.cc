#include "RuntimeModule.h"
#include "Globals.h"
#include "Renderer.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathMap.h"

#include <iostream>
#include <sstream>

typedef SimpleMath::Matrix44f Matrix44f;
typedef SimpleMath::Vector4f Vector4f;
typedef SimpleMath::Matrix33f Matrix33f;
typedef SimpleMath::Vector3f Vector3f;
typedef SimpleMath::MatrixNNf MatrixNNf;
typedef SimpleMath::VectorNf VectorNf;


void handle_keyboard () {
	Camera *active_camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
	assert (active_camera != nullptr);
	Matrix44f camera_view_matrix = SimpleMath::Map<Matrix44f>(active_camera->mtxView, 4, 4);
	Matrix33f camera_rot_inv = camera_view_matrix.block<3,3>(0,0).transpose();

	Vector3f forward = camera_rot_inv.transpose() * Vector3f (0.f, 0.f, 1.f);
	Vector3f right = camera_rot_inv.transpose() * Vector3f (1.f, 0.f, 0.f);
	
	Vector3f eye = SimpleMath::Map<Vector3f>(active_camera->eye, 3, 1);
	Vector3f poi= SimpleMath::Map<Vector3f>(active_camera->poi, 3, 1);

	Vector3f direction (0.f, 0.f, 0.f);

	if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) {
		direction += forward;
	} 
	
	if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) {
		direction -= forward;
	}
	
	if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS) {
		direction += right;
	} 
	
	if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS) {
		direction -= right;
	}

	if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
		direction += Vector3f (0.f, 1.f, 0.f);
	}

	if (glfwGetKey(gWindow, GLFW_KEY_C) == GLFW_PRESS) {
		direction += Vector3f (0.f, -1.f, 0.f);
	}

	float step = 0.1f;
	eye += direction * step;
	poi += direction * step;

	memcpy (active_camera->eye, eye.data(), sizeof(float) * 3);
	memcpy (active_camera->poi, poi.data(), sizeof(float) * 3);
}

// Boilerplate for the module reload stuff

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
	ImGui::Begin("Ddebug");
	if (ImGui::Button("Hallo Katrina")) {
		if (gRenderer->drawDebug) {
			gRenderer->drawDebug = false;
		} else {
			gRenderer->drawDebug = true;
		}
		std::cout << "Clicked on Baem!" << std::endl;
	}
	ImGui::End();


	static bool imgui_test_window = true;
	ImGui::ShowTestWindow();

	float deltaTime = 0.3;
	std::ostringstream s;
	s << "TestModule:  2 Runtime Object 4 " << deltaTime << " update called!";

	handle_keyboard();

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
