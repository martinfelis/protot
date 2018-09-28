#include "RuntimeModule.h"
#include "Globals.h"

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>

#include "RuntimeModuleManager.h"
#include "Serializer.h"
#include "Timer.h"

#include "modules/RenderModule.h"

#include <iostream>
#include <sstream>

using namespace std;

// Boilerplate for the module reload stuff

struct module_state {
};

void handle_mouse (struct module_state *state) {
	if (!glfwGetWindowAttrib(gWindow, GLFW_FOCUSED)) {
		return;
	}

	if (glfwGetMouseButton(gWindow, 1)) {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	} else {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	Camera *camera = &gRenderer->mCamera;
	assert (camera != nullptr);
	const Matrix44f& camera_view_matrix = camera->mViewMatrix;
	Matrix33f camera_rot_inv = camera_view_matrix.block<3,3>(0,0).transpose();

	Vector3f eye = camera->mEye;
	Vector3f poi = camera->mPoi;

	if (glfwGetMouseButton(gWindow, 1)) {
		Vector3f view_dir;

		view_dir = (poi - eye).normalized();
		Vector3f right = camera_rot_inv.block<1,3>(0,0).transpose();
		right = view_dir.cross (Vector3f (0.f, 1.f, 0.f));
		Matrix33f rot_matrix_y = SimpleMath::RotateMat33(
				gGuiInputState->mousedY * 0.4f,
				right[0], right[1], right[2]);
		Matrix33f rot_matrix_x = SimpleMath::RotateMat33(
				gGuiInputState->mousedX * 0.4f,
				0.f, 1.f, 0.f);
		poi = eye + rot_matrix_x * rot_matrix_y * view_dir;

		camera->mPoi = poi;
	}

	camera->UpdateMatrices();
}

void handle_keyboard (struct module_state *state, float dt) {
	if (!glfwGetWindowAttrib(gWindow, GLFW_FOCUSED)) {
		return;
	}

	Camera *camera = &gRenderer->mCamera;
	assert (camera != nullptr);
	const Matrix44f& camera_view_matrix = camera->mViewMatrix;
	Matrix33f camera_rot_inv = camera_view_matrix.block<3,3>(0,0).transpose();

	Vector3f forward = camera_rot_inv.transpose() * Vector3f (0.f, 0.f, 1.f);
	Vector3f right = camera_rot_inv.transpose() * Vector3f (1.f, 0.f, 0.f);

	if (glfwGetMouseButton(gWindow, 1)) {
		// Right mouse button pressed, move the camera
		Vector3f eye = camera->mEye;
		Vector3f poi = camera->mPoi;

		Vector3f direction (0.f, 0.f, 0.f);

		if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) {
			direction -= forward;
		} 

		if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) {
			direction += forward;
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

		if (glfwGetKey(gWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			direction += Vector3f (0.f, -1.f, 0.f);
		}

		eye += direction * 5.f * gTimer->mFrameTime;
		poi += direction * 5.f * gTimer->mFrameTime;

		camera->mEye = eye;
		camera->mPoi = poi;
	} 

	// handle pause
	if (glfwGetKey(gWindow, GLFW_KEY_P) == GLFW_PRESS) {
		gTimer->mPaused = !gTimer->mPaused;	
	} 
}

static struct module_state *module_init() {
	gLog ("%s %s called", __FILE__, __FUNCTION__);
	module_state *state = (module_state*) malloc(sizeof(*state));

	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
}

static void module_finalize(struct module_state *state) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
	free(state);
}

static void module_reload(struct module_state *state, void* read_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);

	// load the state of the entity
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}
}

static void module_unload(struct module_state *state, void* write_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
	// serialize the state of the entity
	if (write_serializer != nullptr) {
		gLog ("Serializing state");
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}

	gLog ("Cleanup complete");
}

static bool module_step(struct module_state *state, float dt) {
	if (gRenderer == nullptr)
		return false;

	handle_mouse(state);
	handle_keyboard(state, dt);

	return true;
}

extern "C" {

const struct module_api MODULE_API = {
	.init = module_init,
	.finalize = module_finalize,
	.reload = module_reload,
	.unload = module_unload,
	.step = module_step
};
}
