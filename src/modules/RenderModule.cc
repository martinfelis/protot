#include "RuntimeModule.h"
#include "Timer.h"
#include "Globals.h"
#include "RenderModule.h"
#include <GLFW/glfw3.h>
#include "Serializer.h"

#include "imgui/imgui.h"
#include "imgui_dock.h"

using namespace SimpleMath::GL;

struct Renderer;

float moving_factor = 1.0f;

struct RendererSettings {
	bool DrawDepth = false;
	bool DrawLightDepth = false;
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
VertexArrayMesh gXZPlaneGrid;
VertexArrayMesh gXZPlaneMesh;
VertexArrayMesh gUnitCubeMesh;
VertexArrayMesh gScreenQuad;

AssetFile gAssetFile;

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
	SerializeBool(*serializer, "protot.RenderModule.DrawLightDepth", sRendererSettings.DrawLightDepth);
	SerializeBool(*serializer, "protot.RenderModule.Camera.mIsOrthographic", gRenderer->mCamera.mIsOrthographic);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFov", gRenderer->mCamera.mFov);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mEye", gRenderer->mCamera.mEye);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mPoi", gRenderer->mCamera.mPoi);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mUp", gRenderer->mCamera.mUp);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mNear", gRenderer->mCamera.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFar", gRenderer->mCamera.mFar);

	SerializeVec3(*serializer, "protot.RenderModule.mLight.mPosition", gRenderer->mLight.mPosition);
	SerializeVec3(*serializer, "protot.RenderModule.mLight.mDirection", gRenderer->mLight.mDirection);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mBBoxSize", gRenderer->mLight.mBBoxSize);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mNear", gRenderer->mLight.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mFar", gRenderer->mLight.mFar);
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
	state->renderer->DrawGui();
	state->renderer->RenderGl();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
}

void Light::UpdateMatrices() {
	mCamera.mIsOrthographic = true;

	mCamera.mEye = mPosition;
	mCamera.mPoi = mPosition - mDirection;
	mCamera.mUp = Vector3f (0.0f, 1.0f, 0.0f);

	mCamera.mProjectionMatrix = Ortho (-mBBoxSize * 0.5f, mBBoxSize * 0.5f, -mBBoxSize * 0.5f, mBBoxSize * 0.5f, mNear, mFar);
	mCamera.mViewMatrix = LookAt (mCamera.mEye, mCamera.mPoi, mCamera.mUp);

	mLightSpaceMatrix = mCamera.mViewMatrix * mCamera.mProjectionMatrix;
}

void Light::DrawGui() {
	ImGui::SliderFloat3("Position", mPosition.data(), -10.0f, 10.0f);
	ImGui::SliderFloat3("Direction", mDirection.data(), -1.0f, 1.0f);
	ImGui::SliderFloat("Volume Size", &mBBoxSize, 1.0f, 50.0f);
	ImGui::SliderFloat("Near", &mNear, -10.0f, 50.0f);
	ImGui::SliderFloat("Far", &mFar, -10.0f, 50.0f);

	ImVec2 content_avail = ImGui::GetContentRegionAvail();

	ImGui::Checkbox("Draw Light Depth", &sRendererSettings.DrawLightDepth);
	void* texture;
	if (sRendererSettings.DrawLightDepth) {
		texture = (void*) mShadowMapTarget.mLinearizedDepthTexture;
	} else {
		texture = (void*) mShadowMapTarget.mColorTexture;
	}

	ImGui::Image(texture,
			ImVec2(content_avail.x, content_avail.x),
			ImVec2(0.0f, 1.0f),
			ImVec2(1.0f, 0.0f)
			);
}

//
// Renderer
//
void Renderer::Initialize(int width, int height) {
	mDefaultTexture.MakeGrid(128, Vector3f (0.8, 0.8f, 0.8f), Vector3f (0.7f, 0.7f, 0.7f));

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

	// Plane Grid
	const int plane_grid_size = 20;
	gXZPlaneGrid.Initialize(gVertexArray, (plane_grid_size + 1) * 4);

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
	gXZPlaneGrid.SetData(plane_data.data(), plane_data.size());

	// Plane Mesh
	gXZPlaneMesh.Initialize(gVertexArray, 4);
	VertexArray::VertexData plane_mesh_vertex_data[] = {
		{-10.0f, 0.0f, -10.0f, 1.0f,		0.0f, 1.0f, 0.0f,		-5.0f, -5.0f,		255, 255, 255, 255},
		{-10.0f, 0.0f,  10.0f, 1.0f,		0.0f, 1.0f, 0.0f,		-5.0f,  5.0f,		255, 255, 255, 255},
		{ 10.0f, 0.0f,  10.0f, 1.0f,		0.0f, 1.0f, 0.0f,	 	 5.0f,  5.0f,		255, 255, 255, 255},
		{ 10.0f, 0.0f, -10.0f, 1.0f,		0.0f, 1.0f, 0.0f,		 5.0f, -5.0f,		255, 255, 255, 255}
	};
	gXZPlaneMesh.SetData(plane_mesh_vertex_data, 4);
	GLuint xz_plane_index_data[] = {
		0, 1, 2,
		2, 3, 0
	};
	gXZPlaneMesh.SetIndexData(xz_plane_index_data, 6);

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

	// Screen Quad
	gScreenQuad.Initialize(gVertexArray, 4);
	VertexArray::VertexData screen_quad_data[] = {
		{-1.0f, -1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,		0.0f, 0.0f,		255, 255, 255, 255},
		{ 1.0f, -1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		255, 255, 255, 255},
		{ 1.0f,  1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,		1.0f, 1.0f,		255, 255, 255, 255},
		{-1.0f,  1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f,		0.0f, 1.0f,		255, 255, 255, 255}
	};
	gScreenQuad.SetData(screen_quad_data, 4);
	GLuint screen_quad_indices[] = {
		0, 1, 2,
		2, 3, 0
	};
	gScreenQuad.SetIndexData(screen_quad_indices, 6);

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
		
	// Program for depth texture rendering
	mRenderQuadProgramDepth = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_depthbuffer.glsl");
	load_result = mRenderQuadProgramDepth.Load();
	mRenderQuadProgramDepth.RegisterFileModification();
	assert(load_result);

	// Render Target
	gLog("Initializing main render target size: %d,%d", width, height);
	mRenderTarget.Initialize(width, height, 
			RenderTarget::EnableColor 
			| RenderTarget::EnableDepthTexture 
			| RenderTarget::EnableLinearizedDepthTexture);

	mRenderTarget.mVertexArray = &gVertexArray;
	mRenderTarget.mQuadMesh = &gScreenQuad;
	mRenderTarget.mLinearizeDepthProgram = mRenderQuadProgramDepth;
	mRenderTarget.mLinearizeDepthProgram.RegisterFileModification();

	// Light
	mLight.Initialize();
	mLight.mShadowMapTarget.mVertexArray = &gVertexArray;
	mLight.mShadowMapTarget.mQuadMesh = &gScreenQuad;
	mLight.mShadowMapTarget.mLinearizeDepthProgram = mRenderQuadProgramDepth;
	mLight.mShadowMapTarget.mLinearizeDepthProgram.RegisterFileModification();

	// Model
	if (!gAssetFile.Load("data/models/RiggedFigure/glTF/RiggedFigure.gltf")) {
		assert(false);
	}
}

void Renderer::Shutdown() {
}


void Renderer::RenderGl() {
	mSceneAreaWidth = mSceneAreaWidth < 1 ? 1 : mSceneAreaWidth;
	mSceneAreaHeight = mSceneAreaHeight < 1 ? 1 : mSceneAreaHeight;
	if (mSceneAreaWidth != mRenderTarget.mWidth || mSceneAreaHeight != mRenderTarget.mHeight) {
		mRenderTarget.Resize(mSceneAreaWidth, mSceneAreaHeight);
	}

	if (mCamera.mWidth != mSceneAreaWidth
			|| mCamera.mHeight != mSceneAreaHeight) {
		mCamera.mWidth = mSceneAreaWidth;
		mCamera.mHeight = mSceneAreaHeight;
	}

	// Shadow Map
	mLight.mShadowMapTarget.Bind();
		glViewport(0, 0, mLight.mShadowMapSize, mLight.mShadowMapSize);
		mLight.UpdateMatrices();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
//		glCullFace(GL_FRONT);
		glUseProgram(mLight.mShadowMapProgram.mProgramId);
		if (mLight.mShadowMapProgram.SetMat44("uLightSpaceMatrix", mLight.mLightSpaceMatrix) == -1) {
			gLog ("Warning: Uniform %s not found!", "uLightSpaceMatrix");
		}
		RenderScene(mLight.mShadowMapProgram, mLight.mCamera);
		mLight.mShadowMapTarget.RenderToLinearizedDepth(mLight.mCamera.mNear, mLight.mCamera.mFar, mLight.mCamera.mIsOrthographic);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glCullFace(GL_BACK);

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
	mRenderTarget.Bind();

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		gLog ("Cannot render: frame buffer invalid!");
	}

	// clear color and depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(mSimpleProgram.mProgramId);
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 0.0f, 0.0f, 1.0f));

	// Coordinate System: VertexArrayMesh
	model_view_projection = 
		TranslateMat44(0.0f, 0.002f, 0.0f)
		* mCamera.mViewMatrix
		* mCamera.mProjectionMatrix;
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (0.0f, 0.0f, 0.0f, 1.0f));
	gVertexArray.Bind();
	gCoordinateFrameMesh.Draw(GL_LINES);

	// Plane
	model_view_projection = 
		TranslateMat44(0.0f, 0.001f, 0.0f)
		* mCamera.mViewMatrix
		* mCamera.mProjectionMatrix;
	mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
	mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 0.4f));
	gVertexArray.Bind();
	gXZPlaneGrid.Draw(GL_LINES);

	// Scene
	glUseProgram(mDefaultProgram.mProgramId);
	glEnable(GL_DEPTH_TEST);

	if (mDefaultProgram.SetMat44("uLightSpaceMatrix", mLight.mLightSpaceMatrix) == -1) {
		gLog ("Warning: Uniform %s not found!", "uLightSpaceMatrix");
	}
	
	RenderScene(mDefaultProgram, mCamera);

	if (mSettings->DrawDepth) {
		mRenderTarget.RenderToLinearizedDepth(mCamera.mNear, mCamera.mFar, mCamera.mIsOrthographic);
	}
}

void Renderer::RenderScene(RenderProgram &program, const Camera& camera) {
	glUseProgram(program.mProgramId);

	Matrix44f model_matrix = TranslateMat44(3.0f, 0.0f, 1.0f);
	Matrix33f normal_matrix = model_matrix.block<3,3>(0,0).transpose();
	normal_matrix = normal_matrix.inverse();

	if (program.SetMat44("uModelMatrix", model_matrix) == -1) {
		gLog ("Warning: could not find uModelMatrix");
	}
	program.SetMat44("uViewMatrix", camera.mViewMatrix);
	program.SetMat44("uProjectionMatrix", camera.mProjectionMatrix);
	program.SetMat33("uNormalMatrix", normal_matrix);

  program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	program.SetVec3("uLightDirection", mLight.mDirection);
	program.SetVec3("uViewPosition", camera.mEye);
	
	program.SetMat44("uLightSpaceMatrix", mLight.mLightSpaceMatrix); 

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mLight.mShadowMapTarget.mDepthTexture);
	program.SetInt("uShadowMap",  0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mDefaultTexture.mTextureId);
	program.SetInt("uAlbedoTexture",  1);

	gVertexArray.Bind();

	program.SetMat44("uModelMatrix", 
			RotateMat44(sin(0.3 * gTimer->mCurrentTime) * 180.0f,
				0.0f, 1.0f, 0.0f)
			* TranslateMat44(3.0, 1.0 + sin(2.0f * gTimer->mCurrentTime), 0.0)) ;
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gUnitCubeMesh.Draw(GL_TRIANGLES);

	program.SetMat44("uModelMatrix", 
			RotateMat44(60.0f, 0.0f, 1.0f, 0.0f)
			* TranslateMat44(-4.0f, 1.0f, 4.0f)
			* ScaleMat44(0.5f, 0.5f, 0.5f));
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gUnitCubeMesh.Draw(GL_TRIANGLES);

	program.SetMat44("uModelMatrix", 
			RotateMat44(60.0f, 0.0f, 1.0f, 0.0f)
			* TranslateMat44(4.0f, 1.0f, 5.0f)
			* ScaleMat44(0.8f, 0.8f, 0.8f));
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gUnitCubeMesh.Draw(GL_TRIANGLES);
	
	program.SetMat44("uModelMatrix", 
			RotateMat44(200.0f, 0.0f, 1.0f, 0.0f)
			* TranslateMat44(moving_factor * sin(gTimer->mCurrentTime), 1.0f, 0.0f)
			* ScaleMat44(0.5f, 0.5f, 0.5f));
	
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gUnitCubeMesh.Draw(GL_TRIANGLES);


	program.SetMat44("uModelMatrix", Matrix44f::Identity());
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gXZPlaneMesh.Draw(GL_TRIANGLES);
}

void Renderer::DrawGui() {
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

	if (ImGui::BeginDock("Light Settings")) {
		mLight.DrawGui();
	}
	ImGui::EndDock();

	if (ImGui::BeginDock("Render Settings")) {
		ImGui::Text("Scene");
		ImGui::SliderFloat("Moving Factor", &moving_factor, -10.0f, 10.0f);

		ImGui::Text("Camera");
		mCamera.DrawGui();

		ImGui::Text("Default Texture");
		ImVec2 content_avail = ImGui::GetContentRegionAvail();
		ImGui::Image((void*) mDefaultTexture.mTextureId, 
				ImVec2(content_avail.x, content_avail.x),
				ImVec2(0.0f, 1.0f), 
				ImVec2(1.0f, 0.0f)
				);
	}
	ImGui::EndDock();

	if (ImGui::BeginDock("Asset")) {
		gAssetFile.DrawGui();
	}
	ImGui::EndDock();
}

