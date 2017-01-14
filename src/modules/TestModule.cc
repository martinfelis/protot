#include "RuntimeModule.h"
#include "Globals.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"
#include <bx/fpumath.h>
#include <GLFW/glfw3.h>
#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathMap.h"
#include "SimpleMath/SimpleMathGL.h"

#include "RuntimeModuleManager.h"
#include "Serializer.h"

#include "modules/RenderModule.h"
#include "modules/CharacterModule.h"

#include <iostream>
#include <sstream>

using namespace std;

double mouse_scroll_x = 0.;
double mouse_scroll_y = 0.;
bool fps_camera = true;

// Boilerplate for the module reload stuff

struct module_state {
	bool fps_camera;
	float camera_theta;
	float camera_phi;
	bool modules_window_visible = false;
	bool imgui_demo_window_visible = false;
	bool character_properties_window_visible = false;
	int modules_window_selected_index = -1;

	CharacterEntity* character = nullptr;
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

	Vector3f eye = active_camera->eye;
	Vector3f poi = active_camera->poi;

	if (glfwGetMouseButton(gWindow, 1)) {
		Vector3f view_dir;

		view_dir = (poi - eye).normalized();
		Vector3f right = camera_rot_inv.block<1,3>(0,0).transpose();
		right = view_dir.cross (Vector3f (0.f, 1.f, 0.f));
		Matrix33f rot_matrix_y = SimpleMath::GL::RotateMat33(
				gRenderer->inputState.mousedY * 0.4f,
				right[0], right[1], right[2]);
		Matrix33f rot_matrix_x = SimpleMath::GL::RotateMat33(
				gRenderer->inputState.mousedX * 0.4f,
				0.f, 1.f, 0.f);
		poi = eye + rot_matrix_x * rot_matrix_y * view_dir;

		active_camera->poi = poi;
	}

	active_camera->updateMatrices();
}

void handle_keyboard (struct module_state *state, float dt) {
	if (!glfwGetWindowAttrib(gWindow, GLFW_FOCUSED)) {
		return;
	}

	if (glfwGetMouseButton(gWindow, 1)) {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	Camera *active_camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
	assert (active_camera != nullptr);
	Matrix44f camera_view_matrix = SimpleMath::Map<Matrix44f>(active_camera->mtxView, 4, 4);
	Matrix33f camera_rot_inv = camera_view_matrix.block<3,3>(0,0).transpose();

	Vector3f forward = camera_rot_inv.transpose() * Vector3f (0.f, 0.f, 1.f);
	Vector3f right = camera_rot_inv.transpose() * Vector3f (1.f, 0.f, 0.f);

	if (glfwGetMouseButton(gWindow, 1)) {
		// Right mouse button pressed, move the camera
		Vector3f eye = active_camera->eye;
		Vector3f poi = active_camera->poi;

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

		eye += direction * 5.f * dt;
		poi += direction * 5.f * dt;

		active_camera->eye = eye;
		active_camera->poi = poi;
	} else if (state->character != nullptr) {
		// Movement of the character
		CharacterController& controller = state->character->controller;

		controller.reset();

		Vector3f forward_plane = Vector3f (0.0f, 1.0f, 0.0f).cross(right);

		// Reset the character control state:
		if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS) {
			controller.direction += forward_plane;
		} 

		if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS) {
			controller.direction -= forward_plane;
		}

		if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS) {
			controller.direction += right;
		} 

		if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS) {
			controller.direction -= right;
		}

		if (glfwGetKey(gWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
			controller.state[CharacterController::ControlStateJump] = true;	
		}
	}
}

void update_character(module_state* state, float dt) {
	if (state->character != nullptr) {
		state->character->update(dt);
	}
}

static struct module_state *module_init() {
	std::cout << "Module init called" << std::endl;
	module_state *state = (module_state*) malloc(sizeof(*state));
	state->character = new CharacterEntity;
	state->character->position = Vector3f (0.f, 0.f, 0.f);
	state->modules_window_selected_index = -1;

	fps_camera = true;

	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
	SerializeVec3(*serializer, "protot.TestModule.entity.position", state->character->position);
	SerializeVec3(*serializer, "protot.TestModule.entity.velocity", state->character->velocity);
	SerializeBool(*serializer, "protot.TestModule.character_window.visible", state->character_properties_window_visible);
	SerializeBool(*serializer, "protot.TestModule.modules_window.visible", state->modules_window_visible);
	SerializeBool(*serializer, "protot.TestModule.imgui_demo_window_visible", state->imgui_demo_window_visible);
	SerializeInt(*serializer, "protot.TestModule.modules_window.selection_index", state->modules_window_selected_index);
}

static void module_finalize(struct module_state *state) {
	std::cout << "Module finalize called" << std::endl;
	free(state);
}

static void module_reload(struct module_state *state, void* read_serializer) {
	std::cout << "Module reload called. State: " << state << std::endl;

	// reset mouse scrolling state
	mouse_scroll_x = 0;
	mouse_scroll_y = 0;

	cout << "Creating render entity ..." << endl;
	// load the state of the entity
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}

	glfwSetScrollCallback (gWindow, mouse_scroll_callback);
}

static void module_unload(struct module_state *state, void* write_serializer) {
	glfwSetScrollCallback (gWindow, nullptr);

	// serialize the state of the entity
	if (write_serializer != nullptr) {
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}

	// clean up
	cout << "destroying render entity " << state->character->entity << endl;
	if (!gRenderer->destroyEntity (state->character->entity)) {
		cerr << "Warning: could not destroy entity " << state->character->entity << endl;
	} else {
		cout << "Successfully destroyed entity " << state->character->entity << endl;

	}
	state->character->entity = nullptr;

	std::cout << "TestModule unloaded. State: " << state << std::endl;
}

void ShowModulesWindow(struct module_state *state) {
	ImGui::SetNextWindowSize (ImVec2(400.f, 300.0f), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos (ImVec2(400.f, 16.0f), ImGuiSetCond_Once);
	ImGui::Begin("Modules");

//	ImGui::Columns(2);
	int selected = state->modules_window_selected_index;
	for (int i = 0; i < gModuleManager->mModules.size(); i++) {
		ImGuiTreeNodeFlags node_flags = 
			ImGuiTreeNodeFlags_Leaf
			| ((i == selected) ? ImGuiTreeNodeFlags_Selected : 0)
			;

		bool node_open = ImGui::TreeNodeEx(
				gModuleManager->mModules[i]->name.c_str(),
				node_flags);

		if (ImGui::IsItemClicked()) {
			selected = i;
		}

		if (node_open) {
			ImGui::TreePop();
		}
	}
	state->modules_window_selected_index = selected;

	ImGui::Separator();

	RuntimeModule* selected_module = nullptr;
	if (selected != -1) {
		selected_module = gModuleManager->mModules[selected];
	}

	if (selected_module) {
		static char time_buf[32];
		memset (time_buf, 0, 32);

		ImGui::LabelText("File", "%s", selected_module->name.c_str());
		ImGui::LabelText("Handle", "0x%p", selected_module->handle);
		ImGui::LabelText("id", "%ld", selected_module->id);
		ImGui::LabelText("mtime", "%ld", selected_module->mtime);
		ImGui::LabelText("mtimensec", "%ld", selected_module->mtimensec);

//		ImGui::LabelText("mtime", "%s", ctime((time_t*)&selected_module->mtime));
//		cout << "time_buf = " << ctime((time_t*)&selected_module->mtime) << endl;

		if (ImGui::Button ("Force Reload")) {
			selected_module->mtime = 0;
			selected_module->id = 0;
		}
	}

	ImGui::End();
}

static bool module_step(struct module_state *state, float dt) {
	if (gRenderer == nullptr)
		return false;

	bool enabled = true;
	static bool imgui_demo_window_visible = false;

	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("Dialogs"))
	{
		ImGui::Checkbox("Modules", &state->modules_window_visible);
		ImGui::Checkbox("ImGui Demo", &state->imgui_demo_window_visible);
		ImGui::Checkbox("Character", &state->character_properties_window_visible);
		
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	if (state->modules_window_visible) {
		ShowModulesWindow(state);
	}

	if (state->character_properties_window_visible && state->character != nullptr) {
		ShowCharacterPropertiesWindow(state->character);
	}

	if (state->imgui_demo_window_visible) {
		ImGui::ShowTestWindow();
	}

	handle_mouse(state);
	handle_keyboard(state, dt);
	update_character(state, dt);

	gRenderer->drawDebugAxes (
			Vector3f (0.f, 0.f, 0.f),
			Matrix33f::Identity(),
			1.0f);

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
