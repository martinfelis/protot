#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Globals.h"

#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include "Timer.h"
#include "RuntimeModuleManager.h"
#include "imgui/imgui.h"
#include "imgui_dock.h"
#include "imgui_impl_glfw_gl3.h"

#include "Serializer.h"

Timer* gTimer = nullptr;
Renderer* gRenderer = nullptr;
GLFWwindow* gWindow = nullptr;
RuntimeModuleManager* gModuleManager = nullptr;
WriteSerializer* gWriteSerializer = nullptr;
ReadSerializer* gReadSerializer = nullptr;
GuiInputState* gGuiInputState = nullptr;
double gTimeAtStart = 0;

double mouse_scroll_x = 0.;
double mouse_scroll_y = 0.;

using namespace std;

// extern "C" {
// GLAPI int gladLoadGL(void);
// }

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	mouse_scroll_x += xoffset;
	mouse_scroll_y += yoffset;
}

void handle_mouse () {
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
	gGuiInputState->mousedX = mouse_x - gGuiInputState->mouseX;
	gGuiInputState->mousedY = mouse_y - gGuiInputState->mouseY;
	gGuiInputState->mouseX = mouse_x;
	gGuiInputState->mouseY = mouse_y;
	gGuiInputState->mouseScroll = mouse_scroll_y;

	gGuiInputState->mouseButton =
		glfwGetMouseButton(gWindow, 0)
		+ (glfwGetMouseButton(gWindow, 1) << 1)
		+ (glfwGetMouseButton(gWindow, 2) << 2);
}

int main(void)
{
	gTimeAtStart = gGetCurrentTime();
	std::cout << "Time at start: " << gTimeAtStart << std::endl;

	LoggingInit();

	WriteSerializer out_serializer;
	ReadSerializer in_serializer;

	// Initialize GLFW
	glfwSetErrorCallback(error_callback);
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 16);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	gWindow = glfwCreateWindow(800, 600, "ProtoT", NULL, NULL);
	glfwMakeContextCurrent(gWindow);
	glfwSwapInterval(1);
	gl3wInit();

	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);

	glfwSetKeyCallback(gWindow, key_callback);
	glfwSetScrollCallback (gWindow, mouse_scroll_callback);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	std::cout << "GLSL Version  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// imgui initialization.
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	GuiInputState gui_input_state;
	gGuiInputState = &gui_input_state;
	ImGui_ImplGlfwGL3_Init(gWindow, false);

	// Timer
	Timer timer;
	gTimer = &timer;
	timer.mCurrentTime = 0.0f;
	timer.mDeltaTime = 0.0f;

	printf("Initializing ModuleManager...\n");
	RuntimeModuleManager module_manager;
	module_manager.RegisterModule("src/modules/libRenderModule.so");
//	module_manager.RegisterModule("src/modules/libCharacterModule.so");
//	module_manager.RegisterModule("src/modules/libTestModule.so");

	// Setup global variables
	gModuleManager = &module_manager;
	gWriteSerializer = &out_serializer;
	gReadSerializer = &in_serializer;

	// Load modules
	module_manager.LoadModules();
	
	double frame_time_last = glfwGetTime();
	double frame_time_current = frame_time_last;
	double frame_delta_time = 0.0;
	uint64_t frame_counter = 0;

	bool draw_imgui_demo = false;

	while(!glfwWindowShouldClose(gWindow)) {
		frame_counter++;

		// Start the imgui frame such that widgets can be submitted
		handle_mouse();
		glfwGetWindowSize(gWindow, &width, &height);

		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();

//		imguiBeginFrame (gGuiInputState->mouseX,
//				gGuiInputState->mouseY,
//				gGuiInputState->mouseButton,
//				gGuiInputState->mouseScroll,
//				width,
//				height);

		if (module_manager.CheckModulesChanged()) {
			gLog("Detected module update at frame %d. Unloading all modules.", frame_counter);
			module_manager.UnloadModules();
			// We need to sleep to make sure we load the new files
			module_manager.LoadModules();
		}

		frame_time_last = frame_time_current;
		frame_time_current = glfwGetTime();
		frame_delta_time = frame_time_current - frame_time_last;
		
		gTimer->mFrameTime = (float)(frame_delta_time);
		if (!gTimer->mPaused) {
			gTimer->mDeltaTime = gTimer->mFrameTime;
			gTimer->mCurrentTime = gTimer->mCurrentTime + gTimer->mDeltaTime;
		} else {
			gTimer->mDeltaTime = 0.0f;
		}

		assert (gTimer->mDeltaTime >= 0.0f);
		int width, height;
		glfwGetWindowSize(gWindow, &width, &height);


		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Dialogs"))
		{
			ImGui::Checkbox("ImGui Demo", &draw_imgui_demo);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		if (draw_imgui_demo)
			ImGui::ShowDemoWindow();

#ifdef USE_DOCKS
		ImGui::SetNextWindowPos(ImVec2(0.0f, 10.0f));
		ImGui::SetNextWindowSize(ImVec2(width, height));
		if (ImGui::Begin("DockArea", NULL,
					ImGuiWindowFlags_NoTitleBar
					| ImGuiWindowFlags_NoResize
					| ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoBringToFrontOnFocus
					)) {
			ImGui::BeginDockspace();

			if (ImGui::BeginDock("dock1")) {
				ImGui::Text("HEllo 1");
			}
			ImGui::EndDock();

			if (ImGui::BeginDock("dock2")) {
				ImGui::Text("HEllo2");
			}
			ImGui::EndDock();

			if (ImGui::BeginDock("dock3")) {
				ImGui::Text("HEllo3");
			}
			ImGui::EndDock();


#endif

			module_manager.Update(gTimer->mDeltaTime);

 #ifdef USE_DOCKS
 			ImGui::EndDockspace();
 		}
 
 		ImGui::End();
 #endif
// 
 		ImGui::Render();

		usleep(16000);

		glfwSwapBuffers(gWindow);
	}

	module_manager.UnregisterModules();

	gRenderer = nullptr;

	ImGui::ShutdownDock();

	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}
