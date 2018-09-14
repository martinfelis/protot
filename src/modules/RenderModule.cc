#include <limits>

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
const int cNumSplits = 4;

struct RendererSettings {
	bool DrawLightDepth = false;
	int RenderMode = 0;
	int ActiveShadowMapSplit = 0;
	int ActiveCameraDebugUi = 0;
};

enum SceneRenderMode {
	SceneRenderModeDefault = 0,
	SceneRenderModeColor = 1,
	SceneRenderModeDepth = 2,
	SceneRenderModeNormals = 3,
	SceneRenderModePositions = 4,
	SceneRenderModeAmbientOcclusion = 5,
	SceneRenderModeCount
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

static void set_icosphere_point(
		VertexArray::VertexData* data,
		unsigned int index,
		const Vector3f &v) {
	assert (index >= 0 && index < 12);

	data[index].x = v[0];
	data[index].y = v[1];
	data[index].z = v[2];
	data[index].w = 1.0f;

	Vector3f vn = v.normalized();
	data[index].nx = vn[0];
	data[index].ny = vn[1];
	data[index].nz = vn[2];

	data[index].s = 0.0f;
	data[index].t = 0.0f;

	data[index].r = 255.0f;
	data[index].g = 255.0f;
	data[index].b = 255.0f;
	data[index].a = 255.0f;
}

VertexArray gVertexArray;
VertexArrayMesh gCoordinateFrameMesh;
VertexArrayMesh gXZPlaneGrid;
VertexArrayMesh gXZPlaneMesh;
VertexArrayMesh gUnitCubeMesh;
VertexArrayMesh gUnitCubeLines;
VertexArrayMesh gScreenQuad;
VertexArrayMesh gIcoSphere;

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
	SerializeBool(*serializer, "protot.RenderModule.DrawLightDepth", sRendererSettings.DrawLightDepth);
	SerializeInt(*serializer, "protot.RenderModule.RenderMode", sRendererSettings.RenderMode);

	SerializeBool(*serializer, "protot.RenderModule.mUseDeferred", gRenderer->mUseDeferred);
	SerializeBool(*serializer, "protot.RenderModule.mIsSSAOEnabled", gRenderer->mIsSSAOEnabled);

	// Camera
	SerializeBool(*serializer, "protot.RenderModule.Camera.mIsOrthographic", gRenderer->mCamera.mIsOrthographic);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFov", gRenderer->mCamera.mFov);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mEye", gRenderer->mCamera.mEye);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mPoi", gRenderer->mCamera.mPoi);
	SerializeVec3(*serializer, "protot.RenderModule.Camera.mUp", gRenderer->mCamera.mUp);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mNear", gRenderer->mCamera.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.Camera.mFar", gRenderer->mCamera.mFar);

	// Debug Camera
	SerializeBool(*serializer, "protot.RenderModule.DebugCamera.mIsOrthographic", gRenderer->mDebugCamera.mIsOrthographic);
	SerializeFloat(*serializer, "protot.RenderModule.DebugCamera.mFov", gRenderer->mDebugCamera.mFov);
	SerializeVec3(*serializer, "protot.RenderModule.DebugCamera.mEye", gRenderer->mDebugCamera.mEye);
	SerializeVec3(*serializer, "protot.RenderModule.DebugCamera.mPoi", gRenderer->mDebugCamera.mPoi);
	SerializeVec3(*serializer, "protot.RenderModule.DebugCamera.mUp", gRenderer->mDebugCamera.mUp);
	SerializeFloat(*serializer, "protot.RenderModule.DebugCamera.mNear", gRenderer->mDebugCamera.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.DebugCamera.mFar", gRenderer->mDebugCamera.mFar);

	SerializeVec3(*serializer, "protot.RenderModule.mLight.mPosition", gRenderer->mLight.mPosition);
	SerializeVec3(*serializer, "protot.RenderModule.mLight.mDirection", gRenderer->mLight.mDirection);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mBBoxSize", gRenderer->mLight.mBBoxSize);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mNear", gRenderer->mLight.mNear);
	SerializeFloat(*serializer, "protot.RenderModule.mLight.mFar", gRenderer->mLight.mFar);

	SerializeBool(*serializer, "protot.RenderModule.mLight.mDebugDrawSplitViewBounds", gRenderer->mLight.mDebugDrawSplitViewBounds);
	SerializeBool(*serializer, "protot.RenderModule.mLight.mDebugDrawSplitWorldBounds", gRenderer->mLight.mDebugDrawSplitWorldBounds);
	SerializeBool(*serializer, "protot.RenderModule.mLight.mDebugDrawSplitLightBounds", gRenderer->mLight.mDebugDrawSplitLightBounds);
	SerializeVec4 (*serializer, "protot.RenderModule.mLight.mSplitBias", gRenderer->mLight.mSplitBias);
	SerializeVec4 (*serializer, "protot.RenderModule.mLight.mShadowSplits", gRenderer->mLight.mShadowSplits);

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

	for (int i = 0; i < cNumSplits; ++i) {
		mSplitTarget[i].Initialize(mShadowMapSize, mShadowMapSize,
				RenderTarget::EnableColor
				| RenderTarget::EnableDepthTexture
				| RenderTarget::EnableLinearizedDepthTexture);
		mSplitTarget[i].mVertexArray = &gVertexArray;
		mSplitTarget[i].mQuadMesh = &gScreenQuad;
	}
}

void Light::UpdateMatrices() {
	mCamera.mIsOrthographic = true;

	mCamera.mEye = mPosition;
	mCamera.mPoi = mPosition + mDirection;
	mCamera.mUp = Vector3f (0.0f, 1.0f, 0.0f);

	mCamera.mProjectionMatrix = Ortho (-mBBoxSize * 0.5f, mBBoxSize * 0.5f, -mBBoxSize * 0.5f, mBBoxSize * 0.5f, mNear, mFar);
	mCamera.mViewMatrix = LookAt (mCamera.mEye, mCamera.mPoi, mCamera.mUp);

	mLightSpaceMatrix = mCamera.mViewMatrix * mCamera.mProjectionMatrix;
}

void Light::DrawGui() {
	ImGui::SliderFloat3("Position", mPosition.data(), -10.0f, 10.0f);
	ImGui::SliderFloat3("Direction", mDirection.data(), -1.0f, 1.0f);
	ImGui::SliderFloat("Near", &mNear, -10.0f, 50.0f);
	ImGui::SliderFloat("Far", &mFar, -10.0f, 50.0f);
	ImGui::SliderFloat4("Shadow Splits Bias", mSplitBias.data(), 0.0f, 0.01f, "%.5f", 1.0f);
	ImGui::SliderFloat4("Shadow Splits", mShadowSplits.data(), 0.01f, 1.0f);

	bool show_cascades = mShowCascadesAlpha == 0.0f ? false : true;
	ImGui::Checkbox("ShowCascades", &show_cascades);
	mShowCascadesAlpha = show_cascades ? 1.0 : 0.0f;

	ImGui::Checkbox("Draw Light Depth", &sRendererSettings.DrawLightDepth);

	ImGui::RadioButton("Split 0", &sRendererSettings.ActiveShadowMapSplit, 0); ImGui::SameLine();
	ImGui::RadioButton("Split 1", &sRendererSettings.ActiveShadowMapSplit, 1); ImGui::SameLine();
	ImGui::RadioButton("Split 2", &sRendererSettings.ActiveShadowMapSplit, 2); ImGui::SameLine();
	ImGui::RadioButton("Split 3", &sRendererSettings.ActiveShadowMapSplit, 3);

	RenderTarget* shadow_split_target = nullptr;
	shadow_split_target = &mSplitTarget[sRendererSettings.ActiveShadowMapSplit];

	void* texture;
	if (sRendererSettings.DrawLightDepth) {
		texture = (void*) shadow_split_target->mLinearizedDepthTexture;
	} else {
		texture = (void*) shadow_split_target->mColorTexture;
	}

	ImVec2 content_avail = ImGui::GetContentRegionAvail();

	ImGui::Image(texture,
			ImVec2(content_avail.x, content_avail.x),
			ImVec2(0.0f, 1.0f),
			ImVec2(1.0f, 0.0f)
			);
}

void Light::UpdateSplits(const Camera& camera) {
	assert(camera.mIsOrthographic == false);

	float near = camera.mNear;
	// Clamp the far plane of the camera so that we only 
	// create shadow maps for things that are relatively near
	// the camera.
	float far = camera.mFar;
	float length = far - near;
	float split_near = near;
	float aspect = camera.mWidth / camera.mHeight;

	float tan_half_hfov = tanf(0.5f * camera.mFov * M_PI / 180.0f);
	float tan_half_vfov = tanf(0.5f * aspect * camera.mFov  * M_PI / 180.0f);

	Vector3f direction = mDirection.normalize();

	Matrix44f look_at = camera.mViewMatrix;
	Matrix44f look_at_inv = look_at.inverse();
	Matrix44f light_matrix = LookAt (mPosition, mPosition + mDirection, Vector3f (0.f, 1.0f, 0.0f));
	Matrix44f light_matrix_inv = light_matrix.inverse();

	mShadowSplits[0] = near + length * 0.02;
	mShadowSplits[1] = near + length * 0.1;
	mShadowSplits[2] = near + length * 0.3;
	mShadowSplits[3] = far;

	float prev_split_far = near;

	for (int i = 0; i < cNumSplits; ++i) {
		split_near = prev_split_far;
		float split_far = mShadowSplits[i];

		mSplitViewFrustum[i] = look_at * Perspective (camera.mFov, aspect, split_near, split_far);

		float xn = split_near * tan_half_vfov;
		float xf = split_far * tan_half_vfov;
		float yn = split_near * tan_half_hfov;
		float yf = split_far * tan_half_hfov;

		Vector4f frustum_corners[] = {
			Vector4f (xn, yn, -split_near, 1.0f),
			Vector4f (-xn, yn, -split_near, 1.0f),
			Vector4f (xn, -yn, -split_near, 1.0f),
			Vector4f (-xn, -yn, -split_near, 1.0f),

			Vector4f (xf, yf, -split_far, 1.0f),
			Vector4f (-xf, yf, -split_far, 1.0f),
			Vector4f (xf, -yf, -split_far, 1.0f),
			Vector4f (-xf, -yf, -split_far, 1.0f)
		};

		BBox bbox_world;
		BBox bbox_light;

		for (int j = 0; j < 8; ++j) {
			Vector4f v_world = look_at_inv.transpose() * frustum_corners[j];
			Vector4f v_light = light_matrix.transpose() * v_world;

			//				gLog("vworld %d: %.3f, %.3f, %.3f, %.3f", j, 
			//						v_world[0],
			//						v_world[1],
			//						v_world[2],
			//						v_world[3]);

			bbox_world.Update(v_world.block<3,1>(0,0));
			bbox_light.Update(v_light.block<3,1>(0,0));
		}

		mSplitBoundsWorldSpace[i] = bbox_world;
		mSplitBoundsLightSpace[i] = bbox_light;

		//			gLog ("min/max %.3f,%.3f, %.3f,%.3f, %.3f,%.3f", 
		//					min_x, max_x,
		//					min_y, max_y,
		//					min_z, max_z
		//					);

		// Compute the light matrices for each frustum
		{
			Vector3f dimensions = (bbox_light.mMax - bbox_light.mMin) * 0.5f;
			Vector3f center = bbox_light.mMin + dimensions;

			mSplitCamera[i].mIsOrthographic = true;
			mSplitCamera[i].mViewMatrix = light_matrix;

			// TODO: values for near and far planes are off
			mSplitCamera[i].mProjectionMatrix = Ortho (
					bbox_light.mMin[0], bbox_light.mMax[0],
					bbox_light.mMin[1], bbox_light.mMax[1],
					//											mLight.mNear, bbox_light.mMax[2]
										mNear, mFar
					//						bbox_light.mMin[2], mLight.mFar
//					bbox_light.mMin[2], bbox_light.mMax[2]
					);

			mSplitLightFrustum[i] = mSplitCamera[i].mViewMatrix
				* mSplitCamera[i].mProjectionMatrix;
			
			prev_split_far = split_far;
		}
	}
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
		{1.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, -1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, -1.0f, -1.0f, 1.0f,	1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 0, 0, 255 },
		{1.0f, 1.0f, -1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 0, 0, 255 },

		// back: -x
		{-1.0f, 1.0f, 1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, -1.0f, 1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, -1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },
		{-1.0f, 1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 0, 0, 255 },

		// side: +z
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 0, 0, 255, 255 },	
		{-1.0f, -1.0f, 1.0f, 1.0f,	0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, -1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 0, 0, 255, 255 },	

		// back side: -z
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{-1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 0, 0, 255, 255 },	

		// top: +y
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 0, 255, 0, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 0, 255, 0, 255 },	
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 0, 255, 0, 255 },	

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

	// Unit Cube (but only lines)
	gUnitCubeLines.Initialize(gVertexArray, 4 * 6);
	VertexArray::VertexData unit_cube_lines_data[] = {
		// front: +x
		{1.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 255, 255, 255 },
		{1.0f, -1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 255, 255, 255 },
		{1.0f, -1.0f, -1.0f, 1.0f,	1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 255, 255, 255 },
		{1.0f, 1.0f, -1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		  0.0f, 0.0f,	255, 255, 255, 255 },

		// back: -x
		{-1.0f, 1.0f, 1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 255, 255, 255 },
		{-1.0f, -1.0f, 1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 255, 255, 255 },
		{-1.0f, -1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 255, 255, 255 },
		{-1.0f, 1.0f, -1.0f, 1.0f,	-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,	255, 255, 255, 255 },

		// side: +z
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, -1.0f, 1.0f, 1.0f,	0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, -1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	

		// back side: -z
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, -1.0f, -1.0f, 1.0f,	0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	

		// top: +y
		{1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, 1.0f, -1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		  0.0f, 0.0f, 255, 255, 255, 255 },	

		// bottom: -y
		{1.0f, -1.0f, 1.0f, 1.0f,		0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{1.0f, -1.0f, -1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, -1.0f, -1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
		{-1.0f, -1.0f, 1.0f, 1.0f,	0.0f, -1.0f, 0.0f,		0.0f, 0.0f, 255, 255, 255, 255 },	
	};


	gUnitCubeLines.SetData(unit_cube_lines_data, 4 * 6);
	// TODO: 
	GLuint unit_cube_lines_index_data[] = {
		0,  1,  1,  2,  2,  3,  3,  0,
		4,  5,  5,  6,  6,  7,  7,  4,
		8,  9,  9, 10, 10, 11, 11,  8,
		12, 13, 13, 14, 14, 15, 15, 12,
		16, 17, 17, 18, 18, 19, 19, 16,
		20, 21, 21, 22, 22, 23, 23, 20,
		12, 14, 13, 15
	};
	gUnitCubeLines.SetIndexData(unit_cube_lines_index_data, 8 * 6 + 4);

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


	// Icosphere
	//
	//

	// Icosphere

	VertexArray::VertexData icosphere_data[12];

	float t_icosphere = (1.0f + sqrt(5.0f)) * 0.5f;
	set_icosphere_point(icosphere_data,  0, Vector3f (-1.0f,  t_icosphere, 0.0f));
	set_icosphere_point(icosphere_data,  1, Vector3f ( 1.0f,  t_icosphere, 0.0f));
	set_icosphere_point(icosphere_data,  2, Vector3f (-1.0f, -t_icosphere, 0.0f));
	set_icosphere_point(icosphere_data,  3, Vector3f ( 1.0f, -t_icosphere, 0.0f));

	set_icosphere_point(icosphere_data,  4, Vector3f (0.0f, -1.0f,  t_icosphere));
	set_icosphere_point(icosphere_data,  5, Vector3f (0.0f,  1.0f,  t_icosphere));
	set_icosphere_point(icosphere_data,  6, Vector3f (0.0f, -1.0f, -t_icosphere));
	set_icosphere_point(icosphere_data,  7, Vector3f (0.0f,  1.0f, -t_icosphere));

	set_icosphere_point(icosphere_data,  8, Vector3f ( t_icosphere, 0.0f, -1.0f));
	set_icosphere_point(icosphere_data,  9, Vector3f ( t_icosphere, 0.0f,  1.0f));
	set_icosphere_point(icosphere_data, 10, Vector3f (-t_icosphere, 0.0f, -1.0f));
	set_icosphere_point(icosphere_data, 11, Vector3f (-t_icosphere, 0.0f,  1.0f));

	gIcoSphere.Initialize(gVertexArray, 12);
	gIcoSphere.SetData(icosphere_data, 12);

	GLuint icosphere_index_data[] = {
		0, 11, 5,
		0, 5, 1,
		0, 1, 7,
		0, 7, 10,
		0, 10, 11,

		1, 5, 9,
		5, 11, 4,
		11, 10, 2,
		10, 7, 6,
		7, 1, 8,

		3, 9, 4,
		3, 4, 2,
		3, 2, 6,
		3, 6, 8,
		3, 8, 9,

		4, 9, 5,
		2, 4, 11,
		6, 2, 10,
		8, 6, 7,
		9, 8, 1
	};
	gIcoSphere.SetIndexData(icosphere_index_data, 20 * 3);

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

	// Deferred geomemtry pass
	mDeferredGeometry = RenderProgram("data/shaders/vs_deferred_geometry.glsl", "data/shaders/fs_deferred_geometry.glsl");
	load_result = mDeferredGeometry.Load();
	mDeferredGeometry.RegisterFileModification();
	assert(load_result);

	mDeferredLighting = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_deferred_lighting.glsl");
	load_result = mDeferredLighting.Load();
	mDeferredLighting.RegisterFileModification();
	assert(load_result);

	// Program for SSAO
	mSSAOProgram = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_ssao.glsl");
	load_result = mSSAOProgram.Load();
	mSSAOProgram.RegisterFileModification();
	assert(load_result);

	mBlurSSAOProgram = RenderProgram("data/shaders/vs_passthrough.glsl", "data/shaders/fs_blur_ssao.glsl");
	load_result = mBlurSSAOProgram.Load();
	mBlurSSAOProgram.RegisterFileModification();
	assert(load_result);

	InitializeSSAOKernelAndNoise();

	// Render Target
	gLog("Initializing main render target size: %d,%d", width, height);
	int render_target_flags = RenderTarget::EnableColor 
			| RenderTarget::EnableDepthTexture 
			| RenderTarget::EnableLinearizedDepthTexture;

	if (mIsSSAOEnabled) {
		render_target_flags = render_target_flags
			| RenderTarget::EnablePositionTexture
			| RenderTarget::EnableNormalTexture;
	}
	mRenderTarget.Initialize(width, height, render_target_flags);

	mRenderTarget.mVertexArray = &gVertexArray;
	mRenderTarget.mQuadMesh = &gScreenQuad;
	mRenderTarget.mLinearizeDepthProgram = mRenderQuadProgramDepth;
	mRenderTarget.mLinearizeDepthProgram.RegisterFileModification();

	// SSAO Target
	mSSAOTarget.Initialize(width, height, RenderTarget::EnableColor);
	mSSAOBlurTarget.Initialize(width, height, RenderTarget::EnableColor);

	mDeferredLightingTarget.Initialize(width, height, RenderTarget::EnableColor);

	// Light
	mLight.Initialize();
	mLight.mShadowMapTarget.mVertexArray = &gVertexArray;
	mLight.mShadowMapTarget.mQuadMesh = &gScreenQuad;
	mLight.mShadowMapTarget.mLinearizeDepthProgram = mRenderQuadProgramDepth;
	mLight.mShadowMapTarget.mLinearizeDepthProgram.RegisterFileModification();

	for (int i = 0; i < cNumSplits; ++i) {
		mLight.mSplitTarget[i].mLinearizeDepthProgram = mRenderQuadProgramDepth;
		mLight.mSplitTarget[i].mLinearizeDepthProgram.RegisterFileModification();
	}

	// Model
	if (!gAssetFile.Load("data/models/RiggedFigure/glTF/RiggedFigure.gltf")) {
		assert(false);
	}
}

void Renderer::Shutdown() {
	if (mSSAONoiseTexture != -1) {
		glDeleteTextures(1, &mSSAONoiseTexture);
	}
}

void Renderer::CheckRenderBuffers() {
	mSceneAreaWidth = mSceneAreaWidth < 1 ? 1 : mSceneAreaWidth;
	mSceneAreaHeight = mSceneAreaHeight < 1 ? 1 : mSceneAreaHeight;

	// TODO: Refactor enabling/disabling buffers for SSAO
	int required_render_flags = RenderTarget::EnableColor 
		| RenderTarget::EnableDepthTexture 
		| RenderTarget::EnableLinearizedDepthTexture;

	if (mIsSSAOEnabled) {
		required_render_flags = required_render_flags
			| RenderTarget::EnablePositionTexture
			| RenderTarget::EnableNormalTexture;
	}

	if (mUseDeferred) {
		required_render_flags = RenderTarget::EnableColor
			| RenderTarget::EnableDepthTexture
			| RenderTarget::EnableNormalTexture

			// TODO: remove these
			| RenderTarget::EnablePositionTexture
			| RenderTarget::EnableLinearizedDepthTexture
			;
	}

	if (mSceneAreaWidth != mRenderTarget.mWidth 
			|| mSceneAreaHeight != mRenderTarget.mHeight
			|| mRenderTarget.mFlags != required_render_flags ) {
		mRenderTarget.Resize(mSceneAreaWidth, mSceneAreaHeight, required_render_flags);

		if (mUseDeferred) {
			mDeferredLightingTarget.Resize(mSceneAreaWidth, mSceneAreaHeight, RenderTarget::EnableColor);
		}
	}

	if (mIsSSAOEnabled 
			&& (mSSAOTarget.mWidth != mSceneAreaWidth 
				|| mSSAOTarget.mHeight != mSceneAreaHeight)) {
		mSSAOTarget.Resize(mSceneAreaWidth, mSceneAreaHeight, RenderTarget::EnableColor);
		mSSAOBlurTarget.Resize(mSceneAreaWidth, mSceneAreaHeight, RenderTarget::EnableColor);
	}
}

void Renderer::RenderGl() {
	CheckRenderBuffers();

	if (mCamera.mWidth != mSceneAreaWidth
			|| mCamera.mHeight != mSceneAreaHeight) {
		mCamera.mWidth = mSceneAreaWidth;
		mCamera.mHeight = mSceneAreaHeight;
		mDebugCamera.mWidth = mSceneAreaWidth;
		mDebugCamera.mHeight = mSceneAreaHeight;
	}

	mDebugCamera.mWidth = 300.0f;
	mDebugCamera.mHeight = 300.0f;
	mDebugCamera.UpdateMatrices();

	mCamera.UpdateMatrices();
	mLight.UpdateMatrices();
	mLight.UpdateSplits(mCamera);

	// Cascaded Shadow Maps
	for (int i = 0; i < cNumSplits; i++) {

		mLight.mSplitTarget[i].Bind();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glUseProgram(mLight.mShadowMapProgram.mProgramId);
		glViewport(0, 0, mLight.mShadowMapSize, mLight.mShadowMapSize);

		if (mLight.mShadowMapProgram.SetMat44("uViewProjectionMatrix", mLight.mSplitLightFrustum[i]) == -1) {
			gLog ("Warning: Uniform %s not found!", "uViewProjectionMatrix");
		}
		
		RenderScene(mLight.mShadowMapProgram, mLight.mSplitCamera[i]);
		mLight.mSplitTarget[i].RenderToLinearizedDepth(mLight.mCamera.mNear, mLight.mCamera.mFar, true);
	}

	// Shadow Map
	mLight.mShadowMapTarget.Bind();
		glViewport(0, 0, mLight.mShadowMapSize, mLight.mShadowMapSize);
		mLight.mShadowMapTarget.Bind();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glUseProgram(mLight.mShadowMapProgram.mProgramId);
		glViewport(0, 0, mLight.mShadowMapSize, mLight.mShadowMapSize);

		if (mLight.mShadowMapProgram.SetMat44("uViewProjectionMatrix", mLight.mLightSpaceMatrix) == -1) {
			gLog ("Warning: Uniform %s not found!", "uViewProjectionMatrix");
		}
		
		RenderScene(mLight.mShadowMapProgram, mLight.mCamera);
		mLight.mShadowMapTarget.RenderToLinearizedDepth(mLight.mCamera.mNear, mLight.mCamera.mFar, true);

	glViewport(0, 0, mCamera.mWidth, mCamera.mHeight);

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

	glEnable(GL_DEPTH_TEST);
	RenderProgram *program = &mDefaultProgram;
	if (mUseDeferred) {
		program = &mDeferredGeometry;
		
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, buffers);
		glClear(GL_COLOR_BUFFER_BIT);
	} else {
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0};
		glDrawBuffers (1, buffers);
	}

	// clear color and depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

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
	mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 0.1f));
	gVertexArray.Bind();
	gXZPlaneGrid.Draw(GL_LINES);

	// Scene
	glUseProgram(program->mProgramId);

	RenderScene(*program, mCamera);

	if (mSettings->RenderMode == SceneRenderModeDepth) {
		mRenderTarget.RenderToLinearizedDepth(mCamera.mNear, mCamera.mFar, mCamera.mIsOrthographic);
	}

	if (mUseDeferred && mIsSSAOEnabled) {
		mSSAOTarget.Bind();
		glViewport(0, 0, mCamera.mWidth, mCamera.mHeight);
		GLenum draw_attachment_0[] = {GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_attachment_0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
	
		// Positions
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mPositionTexture);

		// Normals
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mNormalTexture);

		// Noise
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mSSAONoiseTexture);

		glUseProgram(mSSAOProgram.mProgramId);

		mSSAOProgram.SetInt("uPositions", 0);
		mSSAOProgram.SetInt("uNormals", 1);
		mSSAOProgram.SetInt("uNoise", 2);

		mSSAOProgram.SetFloat("uRadius", mSSAORadius);
		mSSAOProgram.SetFloat("uBias", mSSAOBias);
		mSSAOProgram.SetFloat("uPower", mSSAOPower);
		mSSAOProgram.SetInt("uSampleCount", mSSAOKernel.size());
		mSSAOProgram.SetMat44("uProjection", mCamera.mProjectionMatrix);

		mSSAOProgram.SetVec3Array("uSamples", mSSAOKernel.size(), &mSSAOKernel[0][0]);

		gVertexArray.Bind();
		gScreenQuad.Draw(GL_TRIANGLES);

		// Blur pass
		mSSAOBlurTarget.Bind();
		glViewport(0, 0, mCamera.mWidth, mCamera.mHeight);
		glDrawBuffers(1, draw_attachment_0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mSSAOTarget.mColorTexture);

		glUseProgram(mBlurSSAOProgram.mProgramId);
		mBlurSSAOProgram.SetInt("uAmbientOcclusion", 0);
		mBlurSSAOProgram.SetInt("uBlurSize", mSSAOBlurSize);

		gScreenQuad.Draw(GL_TRIANGLES);
	}

	if (!mIsSSAOEnabled) {
		mSSAOBlurTarget.Bind();
		glViewport(0, 0, mCamera.mWidth, mCamera.mHeight);
		GLenum draw_attachment_0[] = {GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_attachment_0);
		glClearColor(255, 255, 255, 255);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		gScreenQuad.Draw(GL_TRIANGLES);
	}

	if (mUseDeferred) {
		// Deferred: Lighting pass
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0};
		glDrawBuffers (1, buffers);

		mDeferredLightingTarget.Bind();
		glClearColor(0, 0, 0, 255);
		glClear(GL_COLOR_BUFFER_BIT);
	
		glUseProgram(mDeferredLighting.mProgramId);

		// TODO: remove and reconstruct position from depth
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mPositionTexture);
		mDeferredLighting.SetInt("uPosition", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mDepthTexture);
		mDeferredLighting.SetInt("uDepth", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mNormalTexture);
		mDeferredLighting.SetInt("uNormal", 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mRenderTarget.mColorTexture);
		mDeferredLighting.SetInt("uColor", 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mSSAOBlurTarget.mColorTexture);
		mDeferredLighting.SetInt("uAmbientOcclusion", 4);

		// Shadow Map Cascades
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, mLight.mSplitTarget[0].mDepthTexture);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, mLight.mSplitTarget[1].mDepthTexture);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, mLight.mSplitTarget[2].mDepthTexture);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, mLight.mSplitTarget[3].mDepthTexture);

		GLint shadow_maps[4];
		shadow_maps[0] = 5;
		shadow_maps[1] = 6;
		shadow_maps[2] = 7;
		shadow_maps[3] = 8;
		mDeferredLighting.SetIntArray("uShadowMap", cNumSplits, shadow_maps);

		Matrix44f light_matrices[3];

		for (int i = 0; i < cNumSplits; ++i) {
			light_matrices[i] = mCamera.mViewMatrix.inverse() 
				* mLight.mSplitLightFrustum[i];
		}

		mDeferredLighting.SetMat44Array("uViewToLightMatrix", cNumSplits, light_matrices);

		mDeferredLighting.SetFloat("uNear", mCamera.mNear);
		mDeferredLighting.SetFloat("uFar", mCamera.mFar);
		mDeferredLighting.SetVec4("uShadowSplits", mLight.mShadowSplits);
		mDeferredLighting.SetVec4("uShadowSplitBias", mLight.mSplitBias);
		mDeferredLighting.SetFloat("uShowCascadesAlpha", mLight.mShowCascadesAlpha);

		mDeferredLighting.SetMat44("uViewMatrix", mCamera.mViewMatrix);
		Matrix33f view_mat_rot = mCamera.mViewMatrix.block<3,3>(0,0);
		view_mat_rot = view_mat_rot.transpose();
		Vector3f light_direction = view_mat_rot * mLight.mDirection.normalized();

		mDeferredLighting.SetVec3("uLightDirection", light_direction.normalized());
		mDeferredLighting.SetFloat("uShadowBias", mLight.mShadowBias);

		gVertexArray.Bind();
		gScreenQuad.Draw(GL_TRIANGLES);
	}

	if (mDrawDebugCamera) {
		DebugDrawShadowCascades();
	}

	if (mUseDeferred) {
		// Blit the deferred result onto the main render target
//		glBindFramebuffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mDeferredLightingTarget.mFrameBufferId);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mRenderTarget.mFrameBufferId);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0};
		glDrawBuffers (1, buffers);

		glBlitFramebuffer(
				0, mDeferredLightingTarget.mWidth,
				0, mDeferredLightingTarget.mHeight,
				0, mRenderTarget.mWidth,
				0, mRenderTarget.mHeight,
				GL_COLOR_BUFFER_BIT,
				GL_NEAREST
				);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
}

void Renderer::DebugDrawShadowCascades() {
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0};
		glDrawBuffers (1, buffers);

		mDeferredLightingTarget.Bind();

		glUseProgram(mSimpleProgram.mProgramId);
		// Enable wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		gVertexArray.Bind();
		glUseProgram(mSimpleProgram.mProgramId);

		for (int i = 0; i < cNumSplits; ++i) {
			const BBox& bbox_light = mLight.mSplitBoundsLightSpace[i];
			const BBox& bbox_world = mLight.mSplitBoundsWorldSpace[i];

			// Draw view split frustum
			if (mLight.mDebugDrawSplitViewBounds) {
				Matrix44f model_view_projection =
					mLight.mSplitViewFrustum[i].inverse()
					* mCamera.mViewMatrix
					* mCamera.mProjectionMatrix;

				mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
				mSimpleProgram.SetVec4("uColor", Vector4f (1.0f, 0.0f, 1.0f, 1.0f));
				gUnitCubeLines.Draw(GL_LINES);
			}

			// Draw bounding boxes in world space 
			if (mLight.mDebugDrawSplitWorldBounds) {
				Vector3f dimensions = (bbox_world.mMax - bbox_world.mMin) * 0.5f;
				Vector3f center = bbox_world.mMin + dimensions;

				Matrix44f model_view_projection = 
					ScaleMat44 (dimensions[0], dimensions[1], dimensions[2])
					* TranslateMat44(center[0], center[1], center[2])
					* mCamera.mViewMatrix
					* mCamera.mProjectionMatrix;

				mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
				mSimpleProgram.SetVec4("uColor", Vector4f (0.0f, 1.0f, 1.0f, 1.0f));
				gUnitCubeLines.Draw(GL_LINES);
			}

			// Draw bounding boxes in light space
			if (mLight.mDebugDrawSplitLightBounds) {
				Matrix44f model_view_projection = 
					mLight.mSplitLightFrustum[i].inverse()
					* mCamera.mViewMatrix
					* mCamera.mProjectionMatrix;

				mSimpleProgram.SetMat44("uModelViewProj", model_view_projection);
				mSimpleProgram.SetVec4("uColor", Vector4f (0.0f, 0.0f, 1.0f, 1.0f));
				gUnitCubeLines.Draw(GL_LINES);
			}
		}

		// Disable wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
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
			* TranslateMat44(5.0, 1.0 + sin(2.0f * gTimer->mCurrentTime), 0.0)) ;
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
			* TranslateMat44(moving_factor * sin(gTimer->mCurrentTime), 1.0f, -3.0f)
			* ScaleMat44(0.5f, 0.5f, 0.5f));
	
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gUnitCubeMesh.Draw(GL_TRIANGLES);

	program.SetMat44("uModelMatrix", Matrix44f::Identity());
	program.SetVec4("uColor", Vector4f (1.0f, 1.0f, 1.0f, 1.0f));
	gXZPlaneMesh.Draw(GL_TRIANGLES);

	program.SetMat44("uModelMatrix", 
			TranslateMat44(0.0f, 2.0f, 0.0f)
			);
	program.SetVec4("uColor", Vector4f (1.0f, 0.2f, 0.4f, 1.0f));
	gIcoSphere.Draw(GL_TRIANGLES);

	gAssetFile.DrawModel(program);
}

void Renderer::DrawGui() {
	if (ImGui::BeginDock("Scene")) {
		ImGui::RadioButton("Default", &sRendererSettings.RenderMode, 0); ImGui::SameLine();
		ImGui::RadioButton("Color", &sRendererSettings.RenderMode, 1); ImGui::SameLine();
		ImGui::RadioButton("Depth", &sRendererSettings.RenderMode, 2); ImGui::SameLine();
		ImGui::RadioButton("Normals", &sRendererSettings.RenderMode, 3); ImGui::SameLine();
		ImGui::RadioButton("Positions", &sRendererSettings.RenderMode, 4); ImGui::SameLine();
		ImGui::RadioButton("AO", &sRendererSettings.RenderMode, 5);

		switch (sRendererSettings.RenderMode) {
			case SceneRenderModeDefault:	
				mRenderTextureRef.mTextureIdPtr = 
					mUseDeferred ? &mDeferredLightingTarget.mColorTexture : &mRenderTarget.mColorTexture;
				break;
			case SceneRenderModeColor:	
				mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mColorTexture;
				break;
			case SceneRenderModeDepth:	
				mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mLinearizedDepthTexture;
				break;
			case SceneRenderModeNormals:	
				mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mNormalTexture;
				break;
			case SceneRenderModePositions:	
				mRenderTextureRef.mTextureIdPtr = &mRenderTarget.mPositionTexture;
				break;
			case SceneRenderModeAmbientOcclusion:	
				mRenderTextureRef.mTextureIdPtr = &mSSAOBlurTarget.mColorTexture;
				break;
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
		ImGui::Text("Camera");

		mCamera.DrawGui();

		ImGui::Checkbox("Enable Deferred", &mUseDeferred);
		ImGui::Checkbox("Enable SSAO", &mIsSSAOEnabled);

		if (mIsSSAOEnabled) {
			ImGui::SliderFloat("Radius", &mSSAORadius, 0.0f, 1.0f);
			ImGui::SliderFloat("Bias", &mSSAOBias, 0.0f, 0.1f);
			ImGui::SliderFloat("Power", &mSSAOPower, 1.0f, 10.0f);
			ImGui::SliderInt("Samples", &mSSAOKernelSize, 1, 64);
			ImGui::SliderInt("Blur Size", &mSSAOBlurSize, 0, 8);
			ImGui::Checkbox("Disable Color", &mSSAODisableColor);

			if (mSSAOKernelSize != mSSAOKernel.size()) {
				InitializeSSAOKernelAndNoise();
			}
		}
	}
	ImGui::EndDock();

	if (ImGui::BeginDock("Asset")) {
		gAssetFile.DrawGui();
	}
	ImGui::EndDock();
}

void Renderer::InitializeSSAOKernelAndNoise() {
	mSSAOKernel.clear();

	for (unsigned int i = 0; i < mSSAOKernelSize; ++i) {
		Vector3f sample(
				gRandomFloat() * 2.0f - 1.0f,
				gRandomFloat() * 2.0f - 1.0f,
				gRandomFloat()
				);
		sample.normalize();

		// Have a higher distribution of samples close to the origin
		float scale = (float) i / mSSAOKernelSize;
		scale = 0.1 + scale * scale * (1.0f - 0.1);

		mSSAOKernel.push_back(sample * scale);
	}

	std::vector<Vector3f> noise_vectors;
	for (unsigned int i = 0; i < 16; ++i) {
		Vector3f noise(
				gRandomFloat() * 2.0f - 1.0f,
				gRandomFloat() * 2.0f - 1.0f,
				0.0f);
		noise_vectors.push_back(noise);
	}

	if (mSSAONoiseTexture != -1) {
		glDeleteTextures(1, &mSSAONoiseTexture);
	}

	glGenTextures(1, &mSSAONoiseTexture);
	glBindTexture(GL_TEXTURE_2D, mSSAONoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &noise_vectors[0][0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

