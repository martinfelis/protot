//========================================================================
// Simple GLFW example
// Copyright (c) Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
//! [code]

//#define USE_GLAD

#ifdef USE_GLEW
#include <GL/glew.h>
#endif

#ifdef USE_GLAD
#include <glad/glad.h>
#endif

#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "bgfx/bgfxplatform.h"
#include "bx/timer.h"
#include "Renderer.h"
#include "ModuleManager.h"

using namespace std;

static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec3 vCol;\n"
"attribute vec2 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

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

void glfw_simple_example_init () {
}

void glfw_simple_example_draw () {
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

	GLFWwindow* win = glfwCreateWindow(800, 600, "ProtoT", NULL, NULL);
	glfwMakeContextCurrent(win);
	int width, height;
	glfwGetWindowSize(win, &width, &height);

	Renderer renderer;
	bgfx::glfwSetWindow(win);
	bgfx::renderFrame();

	renderer.initialize(width, height);

//	bgfx::init();
//	bgfx::reset(width, height, BGFX_RESET_VSYNC);

//	printf("bgfx renderer is %s\n", bgfx::getRendererName(bgfx::getRendererType()));

	// Enable debug text.
//	bgfx::setDebug(BGFX_DEBUG_TEXT);

	// Set view 0 clear state.
//	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x3070F0FF);

	printf("Initializing ModuleManager...\n");
	ModuleManager module_manager;
	module_manager.init();
	module_manager.RegisterModule("TestModule");

	printf("Starting main loop...\n");
	glfwSetKeyCallback(win, key_callback);
	int64_t time_offset = bx::getHPCounter();

	while(!glfwWindowShouldClose(win)) {
		int64_t now = bx::getHPCounter();
		static int64_t last = now;
		const int64_t frameTime = now - last;
		last = now;
		const double freq = double(bx::getHPFrequency() );
		const double toMs = 1000.0/freq;

		float time = (float)( (now-time_offset)/double(bx::getHPFrequency() ) );

		int width, height;
		glfwGetWindowSize(win, &width, &height);
		if (width != renderer.width || height != renderer.height) {
			renderer.resize(width, height);
		}

		module_manager.update((float)(frameTime / freq));

		renderer.paintGLSimple();

//		bgfx::setViewRect(0, 0, 0, width, height);

		// Dummy submit call to make sure view 0 is cleared
//		bgfx::touch(0);

//		bgfx::dbgTextClear();
//		bgfx::dbgTextPrintf(0, 1, 0x4f, "Test text");
//		bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);
//
//		bgfx::frame();

		glfwPollEvents();
	}
}

//! [code]
