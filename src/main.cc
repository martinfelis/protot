#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "bgfx/bgfxplatform.h"
#include "bx/timer.h"
#include "Timer.h"
#include "RuntimeModuleManager.h"
#include "imgui/imgui.h"

#include "Globals.h"
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

namespace bgfx {
	inline void glfwSetWindow(GLFWwindow* _window)
	{
		bgfx::PlatformData pd;
#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
		pd.ndt          = glfwGetX11Display();
		pd.nwh          = (void*)(uintptr_t)glfwGetGLXWindow(_window);
		pd.context      = glfwGetGLXContext(_window);
#	elif BX_PLATFORM_OSX
		pd.ndt          = NULL;
		pd.nwh          = glfwGetCocoaWindow(_window);
		pd.context      = glfwGetNSGLContext(_window);
#	elif BX_PLATFORM_WINDOWS
		pd.ndt          = NULL;
		pd.nwh          = glfwGetWin32Window(_window);
		pd.context      = NULL;
#	endif // BX_PLATFORM_WINDOWS
		pd.backBuffer   = NULL;
		pd.backBufferDS = NULL;
		bgfx::setPlatformData(pd);
	}
}

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

	WriteSerializer out_serializer;
	ReadSerializer in_serializer;

	// Initialize GLFW
	glfwSetErrorCallback(error_callback);
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 16);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

	gWindow = glfwCreateWindow(800, 600, "ProtoT", NULL, NULL);
	glfwMakeContextCurrent(gWindow);
	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);

	glfwSetKeyCallback(gWindow, key_callback);
	glfwSetScrollCallback (gWindow, mouse_scroll_callback);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	std::cout << "GLSL Version  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Initialize Renderer	
	bgfx::glfwSetWindow(gWindow);
	bgfx::renderFrame();

	uint32_t debug = BGFX_DEBUG_TEXT;
	uint32_t reset = BGFX_RESET_VSYNC;

	bool result = bgfx::init();
	if (!result) {
		std::cerr << "Error: could not initialize renderer!" << std::endl;
		exit (EXIT_FAILURE);
	}

	// imgui initialization.
	imguiCreate();
	GuiInputState gui_input_state;
	gGuiInputState = &gui_input_state;

	// Timer
	Timer timer;
	gTimer = &timer;
	timer.mCurrentTime = 0.0f;
	timer.mDeltaTime = 0.0f;

	printf("Initializing ModuleManager...\n");
	RuntimeModuleManager module_manager;
	module_manager.RegisterModule("src/modules/libRenderModule.so");
	module_manager.RegisterModule("src/modules/libCharacterModule.so");
	module_manager.RegisterModule("src/modules/libTestModule.so");

	// Setup global variables
	gModuleManager = &module_manager;
	gWriteSerializer = &out_serializer;
	gReadSerializer = &in_serializer;

	// Load modules
	module_manager.LoadModules();

	int64_t time_offset = bx::getHPCounter();

	while(!glfwWindowShouldClose(gWindow)) {
		// Start the imgui frame such that widgets can be submitted
		handle_mouse();
		glfwGetWindowSize(gWindow, &width, &height);
		imguiBeginFrame (gGuiInputState->mouseX,
				gGuiInputState->mouseY,
				gGuiInputState->mouseButton,
				gGuiInputState->mouseScroll,
				width,
				height);

		static int64_t last = bx::getHPCounter();
		int64_t pre_module_check = bx::getHPCounter();

		if (module_manager.CheckModulesChanged()) {
			std::cout << "Detected module update. Unloading all modules." << std::endl;
			module_manager.UnloadModules();
			// We need to sleep to make sure we load the new files
			module_manager.LoadModules();
			// We need to update our last timestamp to ignore the delay due
			// to reloading of the modules.
			last = bx::getHPCounter();
		}
	
		// update time that was passed without module reloading
		int64_t now = bx::getHPCounter();
		int64_t module_update = now - pre_module_check;

		int64_t frameTime = (now - last);
		// make sure we do not have negative updates in the very first update
		if (now != last)
			frameTime = frameTime - module_update;

		last = now;
		const double freq = double(bx::getHPFrequency() );
		const double toMs = 1000.0/freq;

		gTimer->mFrameTime = (float)(frameTime / freq);
		if (!gTimer->mPaused) {
			gTimer->mDeltaTime = gTimer->mFrameTime;
			gTimer->mCurrentTime = gTimer->mCurrentTime + gTimer->mDeltaTime;
		} else {
			gTimer->mDeltaTime = 0.0f;
		}

		assert (gTimer->mDeltaTime >= 0.0f);
		module_manager.Update(gTimer->mDeltaTime);

		glfwPollEvents();

		// submit the imgui widgets
		imguiEndFrame();

    usleep(16000);
	}

	module_manager.UnregisterModules();

	gRenderer = nullptr;

	imguiDestroy();
	bgfx::shutdown();
}
