#include <glad/glad.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
//#include <GLFW/glfw3native.h>

#include "Globals.h"

#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <signal.h>

#include "Timer.h"
#include "RuntimeModuleManager.h"
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "FileModificationObserver.h"
#include "Serializer.h"

Timer* gTimer = nullptr;
Renderer* gRenderer = nullptr;
GLFWwindow* gWindow = nullptr;
RuntimeModuleManager* gModuleManager = nullptr;
WriteSerializer* gWriteSerializer = nullptr;
ReadSerializer* gReadSerializer = nullptr;
GuiInputState* gGuiInputState = nullptr;
FileModificationObserver* gFileModificationObserver = nullptr;
double gTimeAtStart = 0;

double mouse_scroll_x = 0.;
double mouse_scroll_y = 0.;

bool show_dock_1 = true;
bool show_dock_2 = true;
bool show_dock_3 = true;

using namespace std;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}

static void opengl_error_callback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam )
{
//	gLog ("OpenGL Error: %s type %0x%x, severity = 0x%x, message = %s",
//           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
//            type, severity, message );
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void signal_handler(int signo) {
	gLog ("Received signal %d", signo);

	if (gModuleManager->CheckModulesChanged()) {
		gModuleManager->UnloadModules();
		gModuleManager->LoadModules();
	}
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	mouse_scroll_x += xoffset;
	mouse_scroll_y += yoffset;
}

void handle_mouse () {
	if (!glfwGetWindowAttrib(gWindow, GLFW_FOCUSED)) {
		return;
	}

	double mouse_x, mouse_y;
	glfwGetCursorPos(gWindow, &mouse_x, &mouse_y);

	if (gGuiInputState->mouseButton) {
		gGuiInputState->mousedX = mouse_x - gGuiInputState->mouseX;
		gGuiInputState->mousedY = mouse_y - gGuiInputState->mouseY;
	} else {
		gGuiInputState->mousedX = 0;
		gGuiInputState->mousedY = 0;
	}
	gGuiInputState->mouseX = mouse_x;
	gGuiInputState->mouseY = mouse_y;
	gGuiInputState->mouseScroll = mouse_scroll_y;

//    if (glfwGetMouseButton(gWindow, 1)) {
//        glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//    } else {
//        glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//    }

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

	if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
		gLog ("Error registering signal handler!");
	}

	WriteSerializer out_serializer;
	ReadSerializer in_serializer;

	// Initialize GLFW
	glfwSetErrorCallback(error_callback);
	glfwInit();

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_SAMPLES, 16);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	gWindow = glfwCreateWindow(800, 600, "ProtoT", NULL, NULL);
	assert(gWindow != NULL);
	glfwMakeContextCurrent(gWindow);
	glfwSwapInterval(1);

	// Initialize OpenGL loader
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

	glfwSetKeyCallback(gWindow, key_callback);
	glfwSetScrollCallback (gWindow, mouse_scroll_callback);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	std::cout << "GLSL Version  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// During init, enable debug output
//	glEnable              ( GL_DEBUG_OUTPUT );
//	glDebugMessageCallback( (GLDEBUGPROC) opengl_error_callback, 0 );

	// imgui initialization.
	IMGUI_CHECKVERSION();	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	GuiInputState gui_input_state;
	gGuiInputState = &gui_input_state;

	ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Setup Style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;                                // When viewports are enabled it is preferable to disable WinodwRounding
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;                   // When viewports are enabled it is preferable to disable WindowBg alpha


	// FileModificationObserver
	FileModificationObserver file_modification_observer;
	gFileModificationObserver = &file_modification_observer;

	// Timer
	Timer timer;
	gTimer = &timer;
	timer.mCurrentTime = 0.0f;
	timer.mDeltaTime = 0.0f;

	printf("Initializing ModuleManager...\n");
	RuntimeModuleManager module_manager;
	module_manager.RegisterModule("src/modules/libRenderModule.so");
	module_manager.RegisterModule("src/modules/libTestModule.so");
//	module_manager.RegisterModule("src/modules/libSceneModule.so");
//	module_manager.RegisterModule("src/modules/libCharacterModule.so");

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

	bool show_demo_window = false;
  	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while(!glfwWindowShouldClose(gWindow)) {
		frame_counter++;

		// Start the imgui frame such that widgets can be submitted
		handle_mouse();

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

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
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Dialogs"))
		{
			ImGui::Checkbox("ImGui Demo", &show_demo_window);
			ImGui::EndMenu();
		}

		float menu_bar_height = ImGui::GetWindowHeight();
		ImGui::EndMainMenuBar();

		if (show_demo_window)
			ImGui::ShowDemoWindow();

        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        bool opt_fullscreen = opt_fullscreen_persistant;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", NULL, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
        } else {
            gLog("Error: no docking not enabled");
        }
        ImGui::End();

		module_manager.Update(gTimer->mDeltaTime);



        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwMakeContextCurrent(gWindow);
		glfwGetFramebufferSize(gWindow, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImDrawData* draw_data = ImGui::GetDrawData();
		assert (draw_data != NULL);
		ImGui_ImplOpenGL3_RenderDrawData(draw_data);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		glfwMakeContextCurrent(gWindow);
		glfwSwapBuffers(gWindow);

		// Send the application to sleep if we have some time left for this frame
		double frame_target_time = 1.0 / module_manager.mTargetFPS;
		if (frame_delta_time < frame_target_time) {
			usleep ((frame_target_time - frame_delta_time) * 1000000 * 0.98);
		}

		if (glfwGetKey(gWindow, GLFW_KEY_F5) == GLFW_PRESS) {
			gFileModificationObserver->Update();
		}
	}

	gLog ("Exiting application");

  module_manager.UnregisterModules();

	gRenderer = nullptr;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}
