#include "RuntimeModule.h"
#include "Globals.h"
#include "modules/RenderModule.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"
#include <bx/fpumath.h>
#include <GLFW/glfw3.h>
#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathMap.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>
#include <sstream>

using namespace std;

typedef SimpleMath::Matrix44f Matrix44f;
typedef SimpleMath::Vector4f Vector4f;
typedef SimpleMath::Matrix33f Matrix33f;
typedef SimpleMath::Vector3f Vector3f;
typedef SimpleMath::MatrixNNf MatrixNNf;
typedef SimpleMath::VectorNf VectorNf;

double mouse_scroll_x = 0.;
double mouse_scroll_y = 0.;
bool fps_camera = true;

// Boilerplate for the module reload stuff

struct module_state {
	bool fps_camera;
	float camera_theta;
	float camera_phi;
};

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	mouse_scroll_x += xoffset;
	mouse_scroll_y += yoffset;
}

void handle_mouse (struct module_state *state) {
	if (!glfwGetWindowAttrib(gWindow, GLFW_FOCUSED)) {
		return;
	}

	if (glfwGetMouseButton(gWindow, 1)) {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	double mouse_x, mouse_y;
	glfwGetCursorPos(gWindow, &mouse_x, &mouse_y);
	gRenderer->inputState.mousedX = mouse_x - gRenderer->inputState.mouseX;
	gRenderer->inputState.mousedY = mouse_y - gRenderer->inputState.mouseY;
	gRenderer->inputState.mouseX = mouse_x;
	gRenderer->inputState.mouseY = mouse_y;
	gRenderer->inputState.mouseScroll = mouse_scroll_y;

	gRenderer->inputState.mouseButton =
		glfwGetMouseButton(gWindow, 0)
		+ (glfwGetMouseButton(gWindow, 1) << 1)
		+ (glfwGetMouseButton(gWindow, 2) << 2);

	Camera *active_camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
	assert (active_camera != nullptr);
	Matrix44f camera_view_matrix = SimpleMath::Map<Matrix44f>(active_camera->mtxView, 4, 4);
	Matrix33f camera_rot_inv = camera_view_matrix.block<3,3>(0,0).transpose();

	Vector3f eye = SimpleMath::Map<Vector3f>(active_camera->eye, 3, 1);
	Vector3f poi = SimpleMath::Map<Vector3f>(active_camera->poi, 3, 1);

	if (glfwGetMouseButton(gWindow, 1)) {
		Vector3f view_dir;

		view_dir = (poi - eye).normalized();
		Vector3f right = camera_rot_inv.block<1,3>(0,0).transpose();
		right = view_dir.cross (Vector3f (0.f, 1.f, 0.f));
		Matrix33f rot_matrix_y = SimpleMath::GL::RotateMat33(
				gRenderer->inputState.mousedY * 0.4f,
				right[0], right[1], right[2]);
		Matrix33f rot_matrix_x = SimpleMath::GL::RotateMat33(
				gRenderer->inputState.mousedX * -0.4f,
				0.f, 1.f, 0.f);
		poi = eye + rot_matrix_x * rot_matrix_y * view_dir;

		memcpy (active_camera->poi, poi.data(), sizeof(float) * 3);
	}

	active_camera->updateMatrices();
}

void handle_keyboard (struct module_state *state) {
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

	if (glfwGetKey(gWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		direction += Vector3f (0.f, -1.f, 0.f);
	}

	float step = 0.1f;
	eye += direction * step;
	poi += direction * step;

	memcpy (active_camera->eye, eye.data(), sizeof(float) * 3);
	memcpy (active_camera->poi, poi.data(), sizeof(float) * 3);
}

static struct module_state *module_init() {
	std::cout << "Module init called" << std::endl;
	module_state *state = (module_state*) malloc(sizeof(*state));

	fps_camera = true;

	return state;
}

static void module_finalize(struct module_state *state) {
	std::cout << "Module finalize called" << std::endl;
	free(state);
}

static void module_reload(struct module_state *state) {
	std::cout << "Module reload called. State: " << state << std::endl;

	// reset mouse scrolling state
	mouse_scroll_x = 0;
	mouse_scroll_y = 0;

	glfwSetScrollCallback (gWindow, mouse_scroll_callback);
}

static void module_unload(struct module_state *state) {
	std::cout << "TestModule unloaded. State: " << state << std::endl;
	glfwSetScrollCallback (gWindow, nullptr);
}

static bool module_step(struct module_state *state) {
	if (gRenderer == nullptr)
		return false;

	bool enabled = true;
	ImGui::Begin("TestModule");
	if (ImGui::Checkbox("FPS Camera", &state->fps_camera)) {
	}

	ImGui::SliderFloat("Theta", &state->camera_theta, -3.141592f, 3.141592f);
	ImGui::SliderFloat("Phi", &state->camera_phi, -3.141592f, 3.141592f);

	if (gRenderer) {
		Camera *active_camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
		if (active_camera) {
			ImGui::SliderFloat3("Eye", active_camera->eye, -30.0f, 30.0f);
			ImGui::SliderFloat3("Poi", active_camera->poi, -30.0f, 30.0f);
		}
		assert (active_camera != nullptr);
	}

	if (ImGui::Button("Hallo Katrina Whaddup?")) {
		if (gRenderer->drawDebug) {
			gRenderer->drawDebug = false;
		} else {
			gRenderer->drawDebug = true;
		}
		std::cout << "Clicked on Baem!" << std::endl;
	}
	ImGui::End();

	static bool imgui_test_window = true;
//	ImGui::ShowTestWindow();

	float deltaTime = 0.3;
	std::ostringstream s;
	s << "TestModule:  2 Runtime Object 4 " << deltaTime << " update called!";

	handle_mouse(state);
	handle_keyboard(state);

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
