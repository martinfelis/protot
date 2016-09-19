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
#include "Renderer.h"
#include "RuntimeModuleManager.h"

#include "Globals.h"
Renderer* gRenderer = nullptr;
GLFWwindow* gWindow = nullptr;

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

int main(void)
{
	// TODO GLFW error checking
	glfwSetErrorCallback(error_callback);
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

	gWindow = glfwCreateWindow(800, 600, "ProtoT", NULL, NULL);
	glfwMakeContextCurrent(gWindow);
	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);

	Renderer renderer;
	bgfx::glfwSetWindow(gWindow);
	bgfx::renderFrame();

	renderer.initialize(width, height);

	gRenderer = &renderer;
	
	printf("Initializing ModuleManager...\n");
	RuntimeModuleManager module_manager;
	module_manager.RegisterModule("libTestModule.so");

	printf("Starting main loop...\n");
	glfwSetKeyCallback(gWindow, key_callback);
	int64_t time_offset = bx::getHPCounter();

	while(!glfwWindowShouldClose(gWindow)) {
		int64_t now = bx::getHPCounter();
		static int64_t last = now;
		const int64_t frameTime = now - last;
		last = now;
		const double freq = double(bx::getHPFrequency() );
		const double toMs = 1000.0/freq;

		float time = (float)( (now-time_offset)/double(bx::getHPFrequency() ) );

		int width, height;
		glfwGetWindowSize(gWindow, &width, &height);
		if (width != renderer.width || height != renderer.height) {
			renderer.resize(width, height);
		}

		module_manager.Update((float)(frameTime / freq));

		renderer.paintGL();

		glfwPollEvents();

		// send inputs to the input state of the renderer
		double mouse_x, mouse_y;
		glfwGetCursorPos(gWindow, &mouse_x, &mouse_y);
		renderer.inputState.mousedX = mouse_x - renderer.inputState.mouseX;
		renderer.inputState.mousedY = mouse_y - renderer.inputState.mouseY;
		renderer.inputState.mouseX = mouse_x;
		renderer.inputState.mouseY = mouse_y;
		renderer.inputState.mouseButton =
			glfwGetMouseButton(gWindow, 0)
			+ (glfwGetMouseButton(gWindow, 1) << 1)
			+ (glfwGetMouseButton(gWindow, 2) << 2);

    usleep(16000);
	}

	module_manager.UnloadModules();

	gRenderer = nullptr;
}

//! [code]
