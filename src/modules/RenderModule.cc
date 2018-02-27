#include "RuntimeModule.h"
#include "Globals.h"
#include "RenderModule.h"
#include <GLFW/glfw3.h>
#include "Serializer.h"

#include "imgui/imgui.h"
#include "imgui_dock.h"

using namespace SimpleMath::GL;

struct Renderer;

struct RendererSettings {
	bool DrawDepth = false;
};

static RendererSettings sRendererSettings;

static const GLfloat g_vertex_buffer_data[] = {
	-0.9f, -0.9f, 0.0f,
	0.9f, -0.9f, 0.0f,
	0.0f, 0.9f, 4.0f
};

static const GLfloat g_quad_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};

static const GLfloat g_textured_quad_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,	0.0f, 1.0f,
	1.0f, -1.0f, 0.0f,	1.0f, 1.0f,
	-1.0f, 1.0f, 0.0f,	0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,	0.0f, 0.0f,
	1.0f, -1.0f, 0.0f,	1.0f, 1.0f,	
	1.0f, 1.0f, 0.0f,		1.0f, 0.0f,
};

static const GLfloat g_coordinate_system_vertex_buffer_data[] = {
	0.0f, 0.0f, 0.0f,		255.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,		255.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,		0.0f, 255.0f, 0.0f,
	0.0f, 1.0f, 0.0f,		0.0f, 255.0f, 0.0f,
	0.0f, 0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f
};

//
// Module
//
struct module_state {
	Renderer *renderer;
};

static struct module_state *module_init() {
	gLog ("%s %s called", __FILE__, __FUNCTION__);
	assert (gWindow != nullptr && "Cannot initialize renderer module without gWindow!");

	module_state *state = (module_state*) malloc(sizeof(*state));
	state->renderer = new Renderer();
	assert (state->renderer != nullptr);

	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
	SerializeBool(*serializer, "protot.RenderModule.DrawDepth", sRendererSettings.DrawDepth);
	SerializeBool(*serializer, "protot.RenderModule.Camera.mIsOrthographic", gRenderer->mCamera.mIsOrthographic);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFov", gRenderer->mCamera.mFov);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mEye", gRenderer->mCamera.mEye);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mPoi", gRenderer->mCamera.mPoi);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mUp", gRenderer->mCamera.mUp);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mNear", gRenderer->mCamera.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFar", gRenderer->mCamera.mFar);

//	SerializeBool (*serializer, "protot.RenderModule.draw_floor", gRenderer->drawFloor);
//	SerializeBool (*serializer, "protot.RenderModule.draw_skybox", gRenderer->drawSkybox);
//	SerializeBool (*serializer, "protot.RenderModule.debug_enabled", gRenderer->drawDebug);
//	SerializeVec3 (*serializer, "protot.RenderModule.camera.eye", camera->eye);
//	SerializeVec3 (*serializer, "protot.RenderModule.camera.poi", camera->poi);
}

static void module_finalize(struct module_state *state) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);

	assert (state->renderer != nullptr);
	delete state->renderer;

	free(state);
}

static void module_reload(struct module_state *state, void *read_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
	assert (gWindow != nullptr);

	gLog ("Renderer initialize");
	assert (state != nullptr);
	state->renderer->Initialize(100, 100);
	state->renderer->mSettings = &sRendererSettings;

	gRenderer = state->renderer;

	// load the state of the module
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}
}

static void module_unload(struct module_state *state, void* write_serializer) {
	// serialize the state of the module
	if (write_serializer != nullptr) {
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}

	gRenderer = nullptr;
	state->renderer->Shutdown();

	gLog ("RenderModule unload called");
}

static bool module_step(struct module_state *state, float dt) {
	int width, height;
	assert (gWindow != nullptr);
	state->renderer->RenderGui();
	state->renderer->RenderGl();

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



//
// Camera
//
void Camera::UpdateMatrices() {
	mViewMatrix = LookAt(mEye, mPoi, mUp);

	if (mIsOrthographic) {
		mProjectionMatrix = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, mNear, mFar);
	} else {
		mProjectionMatrix = Perspective(mFov, mWidth / mHeight, mNear, mFar);
	}
}

void Camera::DrawGui() {
	ImGui::SliderFloat3("Eye", mEye.data(), -10.0f, 10.0f);
	ImGui::SliderFloat3("Poi", mPoi.data(), -10.0f, 10.0f);
	ImGui::SliderFloat3("Up", mUp.data(), -10.0f, 10.0f);
	ImGui::Checkbox("Orthographic", &mIsOrthographic);
	ImGui::SliderFloat("Fov", &mFov, 5, 160);
	ImGui::SliderFloat("Near", &mNear, -10, 10);
	ImGui::SliderFloat("Far", &mFar, -10, 10);
	if (ImGui::Button("Reset")) {
		*this = Camera();
	}
}


//
// Renderer
//
void Renderer::Initialize(int width, int height) {
	mDefaultTexture.MakeGrid(128, Vector3f (0.8, 0.8f, 0.8f), Vector3f (0.2f, 0.2f, 0.2f));
	
	// Mesh
	glGenVertexArrays(1, &mMesh.mVertexArrayId);
	glBindVertexArray(mMesh.mVertexArrayId);
	glGenBuffers(1, &mMesh.mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mMesh.mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// Plane
	glGenVertexArrays(1, &mPlane.mVertexArrayId);
	glBindVertexArray(mPlane.mVertexArrayId);
	glGenBuffers(1, &mPlane.mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mPlane.mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// Coordinate System
	glGenVertexArrays(1, &mCoordinateSystem.mVertexArrayId);
	glBindVertexArray(mCoordinateSystem.mVertexArrayId);
	glGenBuffers(1, &mCoordinateSystem.mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mCoordinateSystem.mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_coordinate_system_vertex_buffer_data), g_coordinate_system_vertex_buffer_data, GL_STATIC_DRAW);

	// Simple Shader
	mDefaultProgram = RenderProgram("data/shaders/vs_simple.glsl", "data/shaders/fs_simple.glsl");
	bool load_result = mDefaultProgram.Load();
	assert(load_result);
	muDefaultModelViewProjection = mDefaultProgram.GetUniformLocation("uModelViewProj");	
	muDefaultColor = mDefaultProgram.GetUniformLocation("uColor");	

	// Render Target
	mRenderTarget = RenderTarget (width, height, 
			RenderTarget::EnableColor 
			| RenderTarget::EnableDepthTexture 
			| RenderTarget::EnableLinearizedDepthTexture);

	// Render Target Quad
	glGenVertexArrays(1, &mRenderQuadVertexArrayId);
	glBindVertexArray(mRenderQuadVertexArrayId);
	
	glGenBuffers(1, &mRenderQuadVertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, mRenderQuadVertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Program for color texture rendering
	mRenderQuadProgramColor = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_simpletexture.glsl");
	load_result = mRenderQuadProgramColor.Load();
	assert(load_result);
	muRenderQuadModelViewProj = mRenderQuadProgramColor.GetUniformLocation("uModelViewProj");
		
	muRenderQuadTexture = mRenderQuadProgramColor.GetUniformLocation("uTexture");
	muRenderQuadTime = mRenderQuadProgramColor.GetUniformLocation("uTime");	

	// Program for depth texture rendering
	mRenderQuadProgramDepth = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_depthbuffer.glsl");
	load_result = mRenderQuadProgramDepth.Load();
	assert(load_result);
	muRenderQuadDepthModelViewProj = mRenderQuadProgramDepth.GetUniformLocation( "uModelViewProj");
	muRenderQuadDepthNear = mRenderQuadProgramDepth.GetUniformLocation("uNear");	
	muRenderQuadDepthFar = mRenderQuadProgramDepth.GetUniformLocation("uFar");	
}

void Renderer::Shutdown() {
	glDeleteVertexArrays(1, &mMesh.mVertexArrayId);
	glDeleteBuffers(1, &mMesh.mVertexBuffer);
	glDeleteVertexArrays(1, &mPlane.mVertexArrayId);
	glDeleteBuffers(1, &mPlane.mVertexBuffer);
	glDeleteVertexArrays(1, &mCoordinateSystem.mVertexArrayId);
	glDeleteBuffers(1, &mCoordinateSystem.mVertexBuffer);
}


void Renderer::RenderGl() {
	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);
	if (width != mWidth || height != mHeight)
		Resize(width, height);

	mCamera.UpdateMatrices();

	Matrix44f model_matrix = TranslateMat44(0.0f, 0.0f, 0.0f);
	Matrix44f model_view_projection = 
		model_matrix
		* mCamera.mViewMatrix
		* mCamera.mProjectionMatrix;

	// enable the render target
	glBindFramebuffer(GL_FRAMEBUFFER, mRenderTarget.mFrameBufferId);
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		gLog ("Cannot render: frame buffer invalid!");
	}

	// clear color and depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(mDefaultProgram.mProgramId);
	glUniformMatrix4fv(muDefaultModelViewProjection, 1, GL_FALSE, model_view_projection.data());
	glUniform4fv(muDefaultColor, 1, Vector4f(1.0f, 0.0f, 0.0f, 1.0f).data());

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mMesh.mVertexBuffer);
	glVertexAttribPointer(
			0,				// attribute 0
			3,				// size
			GL_FLOAT,	// type
			GL_FALSE,	// normalized?
			0,				// stride
			(void*)0	// offset
			);

//	glDrawArrays(GL_TRIANGLES, 0, 3);	// starting from vertex 0; 3 vertices total
	glDisableVertexAttribArray(0);

	glBindAttribLocation(mDefaultProgram.mProgramId, 0, "inCoord");
	glBindAttribLocation(mDefaultProgram.mProgramId, 1, "inColor");

	// Coordinate system
	glBindVertexArray(mCoordinateSystem.mVertexArrayId);
	glEnableVertexAttribArray(0);
	glUniform4fv(muDefaultColor, 1, Vector4f(1.0f, 0.0f, 0.0f, 1.0f).data());
	glBindBuffer(GL_ARRAY_BUFFER, mCoordinateSystem.mVertexBuffer);
	glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(float[6])),
			(void*)0
			);
	glVertexAttribPointer(
			1,
			3,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(float[6])),
			(void*) (sizeof(float[3]))
			);
	glDrawArrays(GL_LINES, 0, 6);

	if (mSettings->DrawDepth) {
		mRenderTarget.RenderToLinearizedDepth(true);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		Matrix44f model_view_projection = Matrix44f::Identity();

		// render depth texture
		glUseProgram(mRenderQuadProgramDepth.mProgramId);
		glUniformMatrix4fv(muRenderQuadModelViewProj, 1, GL_FALSE, model_view_projection.data());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mDepthTexture);
		glUniform1i(muRenderQuadTexture, 0);

		// TODO: adjust for perspective
		glUniform1f(muRenderQuadDepthNear, mCamera.mNear);
		// TODO: why do I have to divide by depth range?
		glUniform1f(muRenderQuadDepthFar, mCamera.mFar / (mCamera.mFar - mCamera.mNear));

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, mRenderQuadVertexBufferId);
		glVertexAttribPointer(
				0,				// attribute 0
				3,				// size
				GL_FLOAT,	// type
				GL_FALSE,	// normalized?
				0,				// stride
				(void*)0	// offset
				);

		glDrawArrays(GL_TRIANGLES, 0, 6);	// starting from vertex 0; 3 vertices total

		mRenderTarget.RenderToLinearizedDepth(false);
	}

	glDisableVertexAttribArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderGui() {
	if (ImGui::BeginDock("Scene")) {
		ImGui::Checkbox("Draw Depth", &mSettings->DrawDepth);

		GLuint texture;
		if (mSettings->DrawDepth) {
			texture = mRenderTarget.mLinearizedDepthTexture;
		} else {
			texture = mRenderTarget.mColorTexture;
		}

		ImGui::Text("Scene");
		const ImVec2 content_avail = ImGui::GetContentRegionAvail();
	
		ImGui::Image((void*) texture,
				content_avail,
				ImVec2(0.0f, 1.0f), 
				ImVec2(1.0f, 0.0f)
				);
	}
	ImGui::EndDock();

	if (ImGui::BeginDock("Render Settings")) {
		ImGui::Text("Camera");
		mCamera.DrawGui();

		ImGui::Text("Default Texture");
		const ImVec2 content_avail = ImGui::GetContentRegionAvail();
		ImGui::Image((void*) mDefaultTexture.mTextureId, 
				ImVec2(content_avail.x, content_avail.x),
				ImVec2(0.0f, 1.0f), 
				ImVec2(1.0f, 0.0f)
				);
	}
	ImGui::EndDock();
}

void Renderer::Resize (int width, int height) {
	mWidth = width;
	mHeight = height;
	mRenderTarget.Resize(mWidth, mHeight);
	mCamera.mWidth = mWidth;
	mCamera.mHeight = mHeight;
	glViewport(0, 0, mWidth, mHeight);
}
