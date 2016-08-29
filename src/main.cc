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

#ifdef USE_GLEW
#include <GL/glew.h>
#endif

#ifdef USE_GLAD
#include <glad/glad.h>
#endif

#include "GLFW/glfw3.h"
#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "bgfx/bgfxplatform.h"
#include "Renderer.h"

using namespace std;

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

	GLFWwindow* win = glfwCreateWindow(800, 600, "Test application", NULL, NULL);
	glfwMakeContextCurrent(win);

	bgfx::glfwSetWindow(win);

	int width, height;
	glfwGetWindowSize(win, &width, &height);

	bgfx::init();
	bgfx::reset(width, height, BGFX_RESET_VSYNC);

	printf("bgfx renderer is %s\n", bgfx::getRendererName(bgfx::getRendererType()));

	// Enable debug text.
	bgfx::setDebug(BGFX_DEBUG_TEXT);

	// Set view 0 clear state.
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x3070F0FF);

	printf("Starting main loop...\n");
	glfwSetKeyCallback(win, key_callback);
	while(!glfwWindowShouldClose(win)) {

		int width, height;
		glfwGetWindowSize(win, &width, &height);
		bgfx::setViewRect(0, 0, 0, width, height);

		// Dummy submit call to make sure view 0 is cleared
		bgfx::touch(0);

		bgfx::dbgTextClear();
		bgfx::dbgTextPrintf(0, 1, 0x4f, "Test text");

		bgfx::frame();
	}
}

//! [code]
