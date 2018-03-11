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
	0.0f, 0.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f
};

VertexArray gVertexArray;
VertexArrayMesh gCoordinateFrameMesh;
VertexArrayMesh gXZPlaneMesh;
VertexArrayMesh gUnitCubeMesh;

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
	SerializeVec3(*serializer, "protot.RenderModule.mLight.mDirection", gRenderer->mLight.mDirection);

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



void Light::Initialize() {
  gLog("Initializing light");
	mShadowMapProgram = RenderProgram("data/shaders/vs_shadowmap.vert", "data/shaders/fs_shadowmap.frag");

	bool load_result = mShadowMapProgram.Load();
	mShadowMapProgram.RegisterFileModification();
	assert(load_result);

	gLog("Initializing light render target size: %d, %d", mShadowMapSize, mShadowMapSize);
	mShadowMapTarget.Initialize(mShadowMapSize, mShadowMapSize,
			RenderTarget::EnableColor
			| RenderTarget::EnableDepthTexture
			| RenderTarget::EnableLinearizedDepthTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, mShadowMapTarget.mFrameBufferId);
	gLog("Framebuffer of light has id: %d", mShadowMapTarget.mFrameBufferId);
	gLog("Initializing light done");
}

void Light::UpdateMatrices() {
	mLightProjection = Ortho (-mBBoxSize * 0.5f, mBBoxSize * 0.5f, -mBBoxSize * 0.5f, mBBoxSize * 0.5f, mNear, mFar);
	mLightView = LookAt(mDirection * mBBoxSize * 0.5f, Vector3f (0.0f, 0.0f, 0.0f), Vector3f (0.0f, 1.0f, 0.0f));
	mLightSpaceMatrix = mLightProjection * mLightView;
}

//
// Renderer
//
void Renderer::Initialize(int width, int height) {
	mDefaultTexture.MakeGrid(128, Vector3f (0.8, 0.8f, 0.8f), Vector3f (0.2f, 0.2f, 0.2f));

	gVertexArray.Initialize(1000, GL_STATIC_DRAW);
	gCoordinateFrameMesh.Initialize(gVertexArray, 6);

	VertexArray::VertexData vertex_data[] = {
		{0.0f, 0.0f, 0.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255  },
		{1.0f, 0.0f, 0.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255  },

		{0.0f, 0.0f, 0.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	  0, 255, 0, 255},
		{0.0f, 1.0f, 0.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	  0, 255, 0, 255},

		{0.0f, 0.0f, 0.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	  0, 0, 255, 255},
		{0.0f, 0.0f, 1.0f, 1.0f,	 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,	  0, 0, 255, 255}
	};

	gCoordinateFrameMesh.SetData(vertex_data, 6);	

	// Plane
	const int plane_grid_size = 20;
	gXZPlaneMesh.Initialize(gVertexArray, (plane_grid_size + 1) * 4);

	std::vector<VertexArray::VertexData> plane_data((plane_grid_size + 1) * 4);
	for (int i = 0, n = plane_grid_size + 1; i < n; ++i) {
		// lines along the x axis
		plane_data[2 * i] = VertexArray::VertexData(
				- plane_grid_size * 0.5f, 0.0f, -plane_grid_size * 0.5f + 1.0f * i, 1.0f,
				0.0f, 1.0f, 0.0f,
				- plane_grid_size * 0.5f, -1.0f * i,
				255, 255, 255, 255);
		plane_data[2 * i + 1] = VertexArray::VertexData(
				plane_grid_size * 0.5f, 0.0f, -plane_grid_size * 0.5 + 1.0f * i, 1.0f,
				0.0f, 1.0f, 0.0f,
				plane_grid_size * 0.5f, -1.0f * i,
				255, 255, 255, 255);

		// lines along the z axis
		plane_data[n * 2 + 2 * i] = VertexArray::VertexData(
				 -plane_grid_size * 0.5f + 1.0f * i, 0.0f, - plane_grid_size * 0.5f, 1.0f,
				0.0f, 1.0f, 0.0f,
				 -1.0f * i, - plane_grid_size * 0.5f,
				255, 255, 255, 255);
		plane_data[n * 2 + 2 * i + 1] = VertexArray::VertexData(
				 -plane_grid_size * 0.5f + 1.0f * i, 0.0f,   plane_grid_size * 0.5f, 1.0f,
				0.0f, 1.0f, 0.0f,
				 -1.0f * i, plane_grid_size * 0.5f,
				255, 255, 255, 255);
	}
	gXZPlaneMesh.SetData(plane_data.data(), plane_data.size());

	// Unit Cube
	gUnitCubeMesh.Initialize(gVertexArray, 4 * 6);
	VertexArray::VertexData unit_cube_data[] = {
		// front: +x
		{1.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, -1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, -1.0f, -1.0f, 1.0f,	1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, 1.0f, -1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },

		// back: -x
		{-1.0f, 1.0f, 1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, -1.0f, 1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, -1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, 1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },

		// side: +z
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{-1.0f, -1.0f, 1.0f, 1.0f,	0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, -1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	

		// back side: -z
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{-1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	

		// top: +y
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	

		// bottom: -y
		{1.0f, -1.0f, 1.0f, 1.0f,		0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{1.0f, -1.0f, -1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, -1.0f, -1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, -1.0f, 1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 0, 255, 0, 255 },	
	};
	gUnitCubeMesh.SetData(unit_cube_data, 4 * 6);
	GLuint unit_cube_index_data[] = {
		0, 1, 2, 2, 3, 0,
		4, 7, 6, 6, 5, 4,
		8, 9, 10, 10, 11, 8,
		12, 15, 14, 14, 13, 12,
		16, 17, 18, 18, 19, 16,
		20, 23, 22, 22, 21, 20
	};
	gUnitCubeMesh.SetIndexData(unit_cube_index_data, 36);

	// Simple Shader
	mSimpleProgram = RenderProgram("data/shaders/vs_simple.glsl", "data/shaders/fs_simple.glsl");
	bool load_result = mSimpleProgram.Load();
	mSimpleProgram.RegisterFileModification();
	assert(load_result);

	mDefaultProgram = RenderProgram("data/shaders/vs_default.glsl", "data/shaders/fs_default.glsl");
	load_result = mDefaultProgram.Load();
	mDefaultProgram.RegisterFileModification();
	assert(load_result);

	// Program for color texture rendering
	mRenderQuadProgramColor = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_simpletexture.glsl");
	load_result = mRenderQuadProgramColor.Load();
	mRenderQuadProgramColor.RegisterFileModification();
	assert(load_result);
	muRenderQuadModelViewProj = mRenderQuadProgramColor.GetUniformLocation("uModelViewProj");
		
	muRenderQuadTexture = mRenderQuadProgramColor.GetUniformLocation("uTexture");
	muRenderQuadTime = mRenderQuadProgramColor.GetUniformLocation("uTime");	

	// Program for depth texture rendering
	mRenderQuadProgramDepth = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_depthbuffer.glsl");
	load_result = mRenderQuadProgramDepth.Load();
	mRenderQuadProgramDepth.RegisterFileModification();
	assert(load_result);
	muRenderQuadDepthModelViewProj = mRenderQuadProgramDepth.GetUniformLocation( "uModelViewProj");
	muRenderQuadDepthNear = mRenderQuadProgramDepth.GetUniformLocation("uNear");	
	muRenderQuadDepthFar = mRenderQuadProgramDepth.GetUniformLocation("uFar");	

	// Render Target
	gLog("Initializing main render target size: %d,%d", width, height);
	mRenderTarget.Initialize(width, height, 
			RenderTarget::EnableColor 
			| RenderTarget::EnableDepthTexture 
			| RenderTarget::EnableLinearizedDepthTexture);

	// Render Target Quad
	glGenVertexArrays(1, &mRenderQuadVertexArrayId);
	glBindVertexArray(mRenderQuadVertexArrayId);
	
	glGenBuffers(1, &mRenderQuadVertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, mRenderQuadVertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	mRenderTarget.mQuadVertexArray = mRenderQuadVertexArrayId;
	mRenderTarget.mQuadVertexBuffer = mRenderQuadVertexBufferId;
	mRenderTarget.mLinearizeDepthProgram = mRenderQuadProgramDepth;

	// Light
	mLight.Initialize();
}

void Renderer::Shutdown() {
}


void Renderer::RenderGl() {
	mSceneAreaWidth = mSceneAreaWidth < 1 ? 1 : mSceneAreaWidth;
	mSceneAreaHeight = mSceneAreaHeight < 1 ? 1 : mSceneAreaHeight;
	if (mSceneAreaWidth != mRenderTarget.mWidth || mSceneAreaHeight != mRenderTarget.mHeight) {
		mRenderTarget.Resize(mSceneAreaWidth, mSceneAreaHeight);
		mCamera.mWidth = mSceneAreaWidth;
		mCamera.mHeight = mSceneAreaHeight;
	}

	// Shadow Map
	glViewport(0, 0, mLight.mShadowMapSize, mLight.mShadowMapSize);
	mLight.mShadowMapTarget.Bind();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glUseProgram(mLight.mShadowMapProgram.mProgramId);
		RenderScene(mLight.mShadowMapProgram);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// TODO: render linearized depth for the light
	// mLight.mShadowMapTarget.RenderToLinearizedDepth(mCamera);

	// Regular rendering
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	glViewport(0, 0, mCamera.mWidth, mCamera.mHeight);
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
	glDisable(GL_DEPTH_TEST);

	glUseProgram(mSimpleProgram.mProgramId);
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 0.0f, 0.0f, 1.0f));

	// Coordinate System: VertexArrayMesh
	model_view_projection = 
		TranslateMat44(0.0f, 0.0f, 0.0f)
		* mCamera.mViewMatrix
		* mCamera.mProjectionMatrix;
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (0.0f, 0.0f, 0.0f, 1.0f));
	gVertexArray.Bind();
	gCoordinateFrameMesh.Draw(GL_LINES);

	// Plane
	model_view_projection = 
		TranslateMat44(0.0f, 0.0f, 0.0f)
		* mCamera.mViewMatrix
		* mCamera.mProjectionMatrix;
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 0.0f, 0.0f, 1.0f));
	gVertexArray.Bind();
	gXZPlaneMesh.Draw(GL_LINES);

	// Scene
	glUseProgram(mDefaultProgram.mProgramId);
	glBindTexture(GL_TEXTURE_2D, mLight.mShadowMapTarget.mDepthTexture);
	glEnable(GL_DEPTH_TEST);

	RenderScene(mDefaultProgram);

	if (mSettings->DrawDepth) {
		mRenderTarget.RenderToLinearizedDepth(mCamera);
	}

	glDisableVertexAttribArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderScene(RenderProgram &program) {
	Matrix44f model_matrix = TranslateMat44(3.0f, 0.0f, 1.0f);
	Matrix33f normal_matrix = model_matrix.block<3,3>(0,0).transpose();
	normal_matrix = normal_matrix.inverse();

	program.SetMat44("uModelMatrix", model_matrix);
	program.SetMat44("uViewMatrix", mCamera.mViewMatrix);
	program.SetMat44("uProjectionMatrix", mCamera.mProjectionMatrix);
	program.SetMat33("uNormalMatrix", normal_matrix);

  program.SetVec4("uColor", Vector4f (1.0f, 0.0f, 0.0f, 1.0f));
	program.SetVec3("uLightDirection", mLight.mDirection);
	program.SetVec3("uViewPosition", mCamera.mEye);
	gVertexArray.Bind();
	gUnitCubeMesh.Draw(GL_TRIANGLES);
}

void Renderer::RenderGui() {
	if (ImGui::BeginDock("Scene")) {
		ImGui::Checkbox("Draw Depth", &mSettings->DrawDepth);

		if (mSettings->DrawDepth) {
			mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mLinearizedDepthTexture;
		} else {
			mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mColorTexture;
		}

		ImGui::Text("Scene");
		const ImVec2 content_avail = ImGui::GetContentRegionAvail();
		mSceneAreaWidth = content_avail.x;
		mSceneAreaHeight = content_avail.y;

		mRenderTextureRef.magic = (GLuint)0xbadface;
		ImGui::Image((void*) &mRenderTextureRef,
				content_avail,
				ImVec2(0.0f, 1.0f), 
				ImVec2(1.0f, 0.0f)
				);
	}
	
	ImGui::EndDock();

	if (ImGui::BeginDock("Render Settings")) {
		ImGui::Text("Light");
		ImGui::SliderFloat3("Direction", mLight.mDirection.data(), -10.0f, 10.0f);
		ImVec2 content_avail = ImGui::GetContentRegionAvail();
		ImGui::Image((void*) mLight.mShadowMapTarget.mLinearizedDepthTexture,
				ImVec2(content_avail.x, content_avail.x),
				ImVec2(0.0f, 1.0f),
				ImVec2(1.0f, 0.0f)
				);
		ImGui::Text("Camera");
		mCamera.DrawGui();

		ImGui::Text("Default Texture");
		content_avail = ImGui::GetContentRegionAvail();
		ImGui::Image((void*) mDefaultTexture.mTextureId, 
				ImVec2(content_avail.x, content_avail.x),
				ImVec2(0.0f, 1.0f), 
				ImVec2(1.0f, 0.0f)
				);
	}
	ImGui::EndDock();
}

