#include "RuntimeModule.h"
#include "Globals.h"
#include "RenderModule.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathMap.h"

#include <assert.h>

#include <bgfx/platform.h>
#include <bx/thread.h>

#include <bx/timer.h>
#include <bx/fpumath.h>
#include <bx/uint32_t.h>
#include <bx/string.h>
#include <dbg.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math_types.h"

#include <string>
#include <iostream>
#include <sstream>

#include "Serializer.h"

using namespace std;
using namespace SimpleMath::GL;

typedef Matrix44f Matrix44f;
typedef Vector4f Vector4f;
typedef Matrix33f Matrix33f;
typedef Vector3f Vector3f;
typedef MatrixNNf MatrixNNf;
typedef VectorNf VectorNf;

struct Renderer;

struct module_state {
	Renderer *renderer;
};

static struct module_state *module_init() {
	gLog ("RenderModule init called.");
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
	// get the state from the serializer
	Camera* camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
	assert (camera != nullptr);

	SerializeBool (*serializer, "protot.RenderModule.draw_floor", gRenderer->drawFloor);
	SerializeBool (*serializer, "protot.RenderModule.draw_skybox", gRenderer->drawSkybox);
	SerializeBool (*serializer, "protot.RenderModule.debug_enabled", gRenderer->drawDebug);
	SerializeVec3 (*serializer, "protot.RenderModule.camera.eye", camera->eye);
	SerializeVec3 (*serializer, "protot.RenderModule.camera.poi", camera->poi);
}

static void module_finalize(struct module_state *state) {
	gLog ("RenderModule finalize called");

	assert (state->renderer != nullptr);
	delete state->renderer;

	free(state);
}

static void module_reload(struct module_state *state, void *read_serializer) {
	gLog ("RenderModule reload called");
	assert (gWindow != nullptr);
	int width, height;
	glfwGetWindowSize(gWindow, &width, &height);

	gLog ("Renderer initialize");
	assert (state != nullptr);
	state->renderer->initialize(width, height);
	gRenderer = state->renderer;

	// load the state of the module
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}

	// get the state from the serializer
	Camera* camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
	assert (camera != nullptr);

	camera->updateMatrices();
}

static void module_unload(struct module_state *state, void* write_serializer) {
	// serialize the state of the module
	if (write_serializer != nullptr) {
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}

	gRenderer = nullptr;
	state->renderer->shutdown();

	gLog ("RenderModule unload called");
}

static bool module_step(struct module_state *state, float dt) {
	int width, height;
	assert (gWindow != nullptr);
	glfwGetWindowSize(gWindow, &width, &height);
	state->renderer->updateShaders();

	bgfx::reset (width, height);

	int dock_top_offset = 20.0f;
	int dock_width = 400;
	ImGui::RootDock(
			ImVec2(width - dock_width, dock_top_offset),
			ImVec2(dock_width, height - dock_top_offset)
			);

	state->renderer->resize (0, 20.0f, width - dock_width, height - 20.0f);

	ImGuizmo::Enable(true);

	state->renderer->paintGL();

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

void Skeleton::UpdateMatrices(const Matrix44f &world_transform) {
	for (uint32_t i = 0; i < mBoneMatrices.size(); ++i) {
		Matrix44f parent_matrix (world_transform);
		if (mParent[i] != -1) {
			parent_matrix = mBoneMatrices[mParent[i]];
		}

		mBoneMatrices[i] = mLocalTransforms[i].toMatrix() * parent_matrix;
	}
}

// BGFX globals
bgfx::VertexBufferHandle cube_vbh;
bgfx::IndexBufferHandle cube_ibh;
bgfx::IndexBufferHandle cube_edges_ibh;
bgfx::VertexBufferHandle plane_vbh;
bgfx::IndexBufferHandle plane_ibh;

bgfx::UniformHandle u_time;
bgfx::UniformHandle u_color;
bgfx::UniformHandle u_line_params;

bgfx::UniformHandle u_mtx;
bgfx::UniformHandle u_exposure;
bgfx::UniformHandle u_flags;
bgfx::UniformHandle u_camPos;
bgfx::UniformHandle s_texCube;
bgfx::UniformHandle s_texCubeIrr;

namespace IBL {
	struct Uniforms
	{
		enum { NumVec4 = 12 };

		void init()
		{
			u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NumVec4);
		}

		void submit()
		{
			bgfx::setUniform(u_params, m_params, NumVec4);
		}

		void destroy()
		{
			bgfx::destroyUniform(u_params);
		}

		union
		{
			struct
			{
				union
				{
					float m_mtx[16];
					/* 0*/ struct { float m_mtx0[4]; };
					/* 1*/ struct { float m_mtx1[4]; };
					/* 2*/ struct { float m_mtx2[4]; };
					/* 3*/ struct { float m_mtx3[4]; };
				};
				/* 4*/ struct { float m_glossiness, m_reflectivity, m_exposure, m_bgType; };
				/* 5*/ struct { float m_metalOrSpec, m_unused5[3]; };
				/* 6*/ struct { float m_doDiffuse, m_doSpecular, m_doDiffuseIbl, m_doSpecularIbl; };
				/* 7*/ struct { float m_cameraPos[3], m_unused7[1]; };
				/* 8*/ struct { float m_rgbDiff[4]; };
				/* 9*/ struct { float m_rgbSpec[4]; };
				/*10*/ struct { float m_lightDir[3], m_unused10[1]; };
				/*11*/ struct { float m_lightCol[3], m_unused11[1]; };
			};

			float m_params[NumVec4*4];
		};

		bgfx::UniformHandle u_params;
	};

	struct Settings
	{
		Settings()
		{
			m_envRotCurr = 0.0f;
			m_envRotDest = 0.0f;
			m_lightDir[0] = -0.8f;
			m_lightDir[1] = 0.2f;
			m_lightDir[2] = -0.5f;
			m_lightCol[0] = 1.0f;
			m_lightCol[1] = 1.0f;
			m_lightCol[2] = 1.0f;
			m_glossiness = 0.7f;
			m_exposure = 0.0f;
			m_bgType = 3.0f;
			m_radianceSlider = 2.0f;
			m_reflectivity = 0.85f;
			m_rgbDiff[0] = 1.0f;
			m_rgbDiff[1] = 1.0f;
			m_rgbDiff[2] = 1.0f;
			m_rgbSpec[0] = 1.0f;
			m_rgbSpec[1] = 1.0f;
			m_rgbSpec[2] = 1.0f;
			m_lod = 0.0f;
			m_doDiffuse = false;
			m_doSpecular = false;
			m_doDiffuseIbl = true;
			m_doSpecularIbl = true;
			m_showLightColorWheel = true;
			m_showDiffColorWheel = true;
			m_showSpecColorWheel = true;
			m_metalOrSpec = 0;
			m_meshSelection = 0;
			m_crossCubemapPreview = ImguiCubemap::Latlong;
		}

		float m_envRotCurr;
		float m_envRotDest;
		float m_lightDir[3];
		float m_lightCol[3];
		float m_glossiness;
		float m_exposure;
		float m_radianceSlider;
		float m_bgType;
		float m_reflectivity;
		float m_rgbDiff[3];
		float m_rgbSpec[3];
		float m_lod;
		bool m_doDiffuse;
		bool m_doSpecular;
		bool m_doDiffuseIbl;
		bool m_doSpecularIbl;
		bool m_showLightColorWheel;
		bool m_showDiffColorWheel;
		bool m_showSpecColorWheel;
		uint8_t m_metalOrSpec;
		uint8_t m_meshSelection;
		ImguiCubemap::Enum m_crossCubemapPreview;
	};

	Settings settings;
	Uniforms uniforms;
};


int64_t m_timeOffset;

// 
// Vertex packing utilities
//

uint32_t packUint32(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w)
{
	union
	{
		uint32_t ui32;
		uint8_t arr[4];
	} un;

	un.arr[0] = _x;
	un.arr[1] = _y;
	un.arr[2] = _z;
	un.arr[3] = _w;

	return un.ui32;
}

uint32_t packF4u(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const uint8_t xx = uint8_t(_x*127.0f + 128.0f);
	const uint8_t yy = uint8_t(_y*127.0f + 128.0f);
	const uint8_t zz = uint8_t(_z*127.0f + 128.0f);
	const uint8_t ww = uint8_t(_w*127.0f + 128.0f);
	return packUint32(xx, yy, zz, ww);
}

//
// Render programs
//
bool RenderProgram::checkModified() const {
	// early out if we don't have filenames
	if (vertexShaderFileName == ""
			|| fragmentShaderFileName == "") {
		return false;
	}

	struct stat attr;
	bool stat_result = false;

	// check mtime of vertex shader
	stat_result = stat(vertexShaderFileName.c_str(), &attr);
	if (stat_result == 0
			&& attr.st_mtime != vertexShaderFileModTime) {
		return true;
	}

	// check mtime of fragment shader
	stat_result = stat(fragmentShaderFileName.c_str(), &attr);
	if (stat_result == 0
			&& attr.st_mtime != fragmentShaderFileModTime) {
		return true;
	}

	return false;
}

bool RenderProgram::reload() {
	int vertex_mtime = -1;
	int fragment_mtime = -1;
	struct stat attr;
	bool stat_result = false;

	// get mtime of vertex shader
	stat_result = stat(vertexShaderFileName.c_str(), &attr);
	if (stat_result == 0)
		vertex_mtime = attr.st_mtime;

	stat_result = stat(fragmentShaderFileName.c_str(), &attr);
	if (stat_result == 0)
		fragment_mtime = attr.st_mtime;

	// we need to update the mod times, otherwise we keep reloading
	// the same faulty files again and again.
	vertexShaderFileModTime = vertex_mtime;
	fragmentShaderFileModTime = fragment_mtime;

	bgfx::ProgramHandle new_handle = 
		bgfxutils::loadProgramFromFiles (
				vertexShaderFileName.c_str(),
				fragmentShaderFileName.c_str()
				);

	if (bgfx::isValid(new_handle)) {
		if (bgfx::isValid(program)) {
			bgfx::destroyProgram(program);
		}

		gLog ("Reload of shaders %s and %s success!", 
				vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

		program = new_handle;
		return true;
	} else {
		gLog ("Reload of shaders %s and %s failed!", 
				vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
	}

	return false;
}

// 
// Render states
//

RenderState s_renderStates[RenderState::Count] = {
	{ // Skybox
   	0 
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::Skybox
	},
	{ // ShadowMap
   	0 
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::ShadowMap
	},
	{ // Scene
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),	
		RenderState::Scene
	},
	{ // SceneTextured
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::SceneTextured
	},
	{ // Lines
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
//		| BGFX_STATE_DEPTH_TEST_GREATER
//		| BGFX_STATE_PT_LINES
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::Lines
	},
	{ // LinesOccluded
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_TEST_GREATER
//		| BGFX_STATE_PT_LINES
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::LinesOccluded
	},

	{ // Debug
		0	
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_PT_LINES
//		| BGFX_STATE_PT_POINTS
		| BGFX_STATE_MSAA,
		0,
		RenderProgram(),
		RenderState::Debug
	}
};

// 
// Vertex formats
//

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorVertex::ms_decl;

struct PosNormalVertex
{
	float    m_x;
	float    m_y;
	float    m_z;
	uint32_t m_normal;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosNormalVertex::ms_decl;

struct PosNormalColorTexcoordVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_normal;
	uint32_t m_color;
	float m_u;
	float m_v;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, false)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosNormalColorTexcoordVertex::ms_decl;

struct PosColorTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_rgba;
	float m_u;
	float m_v;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorTexCoord0Vertex::ms_decl;

// Used for lines
struct PathVertex
{
	float position[3];
	float prev[3];
	float next[3];
	// TODO: use an int for the direction
	float info[2]; // direction and length in screen space
	float color[4];

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord1, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord2, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};
bgfx::VertexDecl PathVertex::ms_decl;



// 
// Static geometries
// 

// Plane
PosNormalColorTexcoordVertex s_hplaneVertices[] =
{
	{ -1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f),      0.f,      0.f },
	{  1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f), 10.f,      0.f },
	{ -1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f),     0.f,  10.f},
	{  1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f), 10.f,  10.f },
};

const uint16_t s_planeIndices[] =
{
	0, 1, 2,
	1, 3, 2,
};

// Cube
PosColorVertex s_cubeVertices[8] =
{
	{-1.0f,  1.0f,  1.0f, 0xffffffff },
	{ 1.0f,  1.0f,  1.0f, 0xffffffff },
	{-1.0f, -1.0f,  1.0f, 0xffffffff },
	{ 1.0f, -1.0f,  1.0f, 0xffffffff },
	{-1.0f,  1.0f, -1.0f, 0xffffffff },
	{ 1.0f,  1.0f, -1.0f, 0xffffffff },
	{-1.0f, -1.0f, -1.0f, 0xffffffff },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

const uint16_t s_cubeEdgeIndices[24] =
{
	0, 1,
	1, 3,
	3, 2,
	2, 0,

	4, 5,
	5, 7,
	7, 6,
	6, 4,

	0, 4,
	1, 5,
	2, 6,
	3, 7
};

const uint16_t s_cubeIndices[36] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};

bool flipV = false;
static float s_texelHalf = 0.0f;

void screenSpaceQuad(float _textureWidth, float _textureHeight, bool _originBottomLeft = false, float _width = 1.0f, float _height = 1.0f)
{
	if (bgfx::getAvailTransientVertexBuffer(3, PosColorTexCoord0Vertex::ms_decl) )
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, PosColorTexCoord0Vertex::ms_decl);
		PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

		const float zz = 0.0f;

		const float minx = -_width;
		const float maxx =  _width;
		const float miny = 0.0f;
		const float maxy = _height*2.0f;

		const float texelHalfW = s_texelHalf/_textureWidth;
		const float texelHalfH = s_texelHalf/_textureHeight;
		const float minu = -1.0f + texelHalfW;
		const float maxu =  1.0f + texelHalfW;

		float minv = texelHalfH;
		float maxv = 2.0f + texelHalfH;

		if (_originBottomLeft)
		{
			std::swap(minv, maxv);
			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_rgba = 0xffffffff;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_rgba = 0xffffffff;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_rgba = 0xffffffff;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		bgfx::setVertexBuffer(&vb);
	}
}

void Camera::updateMatrices() {
	assert (width != -1.f && height != -1.f);

	// view matrix
	bx::mtxLookAtRh (mtxView, eye.data(), poi.data(), up.data());

	// projection matrix
	if (orthographic) {
		bx::mtxOrthoRh(mtxProj, 
				-width * 0.5f, width * 0.5f,
				-height * 0.5f, height * 0.5f, 
				near, far);
	} else {
		float aspect = static_cast<float>(width) / height;
		float mtx_proj[16];
		bx::mtxProjRh(mtxProj, fov, aspect, near, far);
	}

	// environment matrix
	const float dir[3] =
	{
		poi[0] - eye[0],
		poi[1] - eye[1],
		poi[2] - eye[2]
	};

	const float dirLen = bx::vec3Length(dir);
	const float invDirLen = 1.0f / (dirLen + FLT_MIN);

	const float dirNorm[3] =
	{
		dir[0] * invDirLen,
		dir[1] * invDirLen,
		dir[2] * invDirLen
	};

	float tmp[3];
	const float fakeUp[3] = { 0.0f, 1.0f, 0.0f };
	float right[3];
	bx::vec3Cross (tmp, fakeUp, dirNorm);
	bx::vec3Norm(right, tmp);

	float up[3];
	bx::vec3Cross(tmp, dirNorm, right);
	bx::vec3Norm(up, tmp);

	mtxEnv[ 0] = -right[0];
	mtxEnv[ 1] = -right[1];
	mtxEnv[ 2] = -right[2];
	mtxEnv[ 3] = 0.0f;
	mtxEnv[ 4] = up[0];
	mtxEnv[ 5] = up[1];
	mtxEnv[ 6] = up[2];
	mtxEnv[ 7] = 0.0f;
	mtxEnv[ 8] = dirNorm[0];
	mtxEnv[ 9] = dirNorm[1];
	mtxEnv[10] = dirNorm[2];
	mtxEnv[11] = 0.0f;
	mtxEnv[12] = 0.0f;
	mtxEnv[13] = 0.0f;
	mtxEnv[14] = 0.0f;
	mtxEnv[15] = 1.0f;
}

void LightProbe::load(const char* _name) {
	char filePath[512];

	bx::snprintf(filePath, BX_COUNTOF(filePath), "data/textures/%s_lod.dds", _name);
	m_tex = bgfxutils::loadTexture(filePath, BGFX_TEXTURE_U_CLAMP|BGFX_TEXTURE_V_CLAMP|BGFX_TEXTURE_W_CLAMP);

	bx::snprintf(filePath, BX_COUNTOF(filePath), "data/textures/%s_irr.dds", _name);
	m_texIrr = bgfxutils::loadTexture(filePath, BGFX_TEXTURE_U_CLAMP|BGFX_TEXTURE_V_CLAMP|BGFX_TEXTURE_W_CLAMP);
}

void Renderer::createGeometries() {
	// Create vertex stream declaration.
	PosColorVertex::init();
	PosNormalVertex::init();
	PosNormalColorTexcoordVertex::init();
	PosColorTexCoord0Vertex::init();
	PathVertex::init();

	// Create static vertex buffer.
	cube_vbh = bgfx::createVertexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) )
			, PosColorVertex::ms_decl
			);

	// Create static index buffer.
	cube_ibh = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) )
			);

	// Create static index buffer.
	cube_edges_ibh = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeEdgeIndices, sizeof(s_cubeEdgeIndices) )
			);

	plane_vbh = bgfx::createVertexBuffer(
			bgfx::makeRef(s_hplaneVertices, sizeof(s_hplaneVertices) )
			, PosNormalColorTexcoordVertex::ms_decl
			);

	plane_ibh = bgfx::createIndexBuffer(
			bgfx::makeRef(s_planeIndices, sizeof(s_planeIndices) )
			);

//	debugBoneMesh.Merge(
//			*Mesh::sCreateCuboid (1., 1., 1.),
//			TranslateMat44(0.f, .0f, 0.5f)
//			);

	Mesh* sphere = Mesh::sCreateUVSphere (4, 4, 0.1f);

	debugBoneMesh.Merge(
			*sphere,
			TranslateMat44(0.f, .0f, 0.0f)
			);

	debugBoneMesh.Merge(
			*sphere,
			TranslateMat44(0.f, .0f, 1.0f)
			);

	Mesh* cuboid = Mesh::sCreateCuboid (0.1f, 0.1f, 1.0f);
	debugBoneMesh.Merge(
			*cuboid,
			TranslateMat44(0.f, .0f, 0.5f)
			);

	delete sphere;
	debugBoneMesh.Update();
	delete cuboid;
}

Path::~Path() {
	if (bgfx::isValid(mVertexBufferHandle))
		bgfx::destroyDynamicVertexBuffer(mVertexBufferHandle);

	if (bgfx::isValid(mIndexBufferHandle))
		bgfx::destroyDynamicIndexBuffer(mIndexBufferHandle);
}

void Path::UpdateBuffers() {
	// create an array for the actual buffer
	std::vector<PathVertex> path_vertices;


	float path_length = 0;

	// Copy points to the vertex buffer
	for (int i = 0; i < points.size(); i++) {
		PathVertex vertex;
		memcpy (vertex.position, points[i].data(), sizeof(float) * 3);
		memcpy (vertex.color, colors[i].data(), sizeof(float) * 4);
		if (i == 0) {
			assert (points.size() > 1);
			memcpy (vertex.prev, points[i].data(), sizeof(float) * 3);
			memcpy (vertex.next, points[i + 1].data(), sizeof(float) * 3);
		} else if (i == points.size() - 1) {
			memcpy (vertex.next, points[i].data(), sizeof(float) * 3);
			memcpy (vertex.prev, points[i - 1].data(), sizeof(float) * 3);
		} else { 
			memcpy (vertex.prev, points[i - 1].data(), sizeof(float) * 3);
			memcpy (vertex.next, points[i + 1].data(), sizeof(float) * 3);
		}

		if (i > 0) {
			path_length += (points[i] - points[i - 1]).norm();
		}
		vertex.info[0] = -1.0f;
		vertex.info[1] = path_length;
		path_vertices.push_back(vertex);
		vertex.info[0] =  1.0f;
		vertex.info[1] = path_length;
		path_vertices.push_back(vertex);

		PathVertex &last_vertex = path_vertices[path_vertices.size() - 1];
		assert (last_vertex.color[0] == colors[i][0]);
		assert (last_vertex.color[1] == colors[i][1]);
		assert (last_vertex.color[2] == colors[i][2]);
		assert (last_vertex.color[3] == 1.0f);
	}

	// Assemble the index buffer (every point is submitted twice
	// and gets extruded in the vertex shader)
	std::vector<uint16_t> line_indices;
	int index = 0;
	for (int i = 0; i < (path_vertices.size() / 2) - 1; i++) {
		int j = index;
		line_indices.push_back(j + 0);
		line_indices.push_back(j + 1);
		line_indices.push_back(j + 2);
		line_indices.push_back(j + 2);
		line_indices.push_back(j + 1);
		line_indices.push_back(j + 3);
		index += 2;
	}

	// create buffers if they are not yet valid
	if (!bgfx::isValid(mVertexBufferHandle)) {
		mVertexBufferHandle = bgfx::createDynamicVertexBuffer(
				bgfx::copy(
					path_vertices.data(), 
					sizeof(PathVertex) * path_vertices.size()
					),
				PathVertex::ms_decl
				);

		mIndexBufferHandle = bgfx::createDynamicIndexBuffer(
				bgfx::copy(line_indices.data(), sizeof(uint16_t) * line_indices.size())
				);

	} else {
		bgfx::updateDynamicVertexBuffer(
				mVertexBufferHandle,
				0,
				bgfx::copy(
					path_vertices.data(), 
					sizeof(PathVertex) * path_vertices.size()
					)
				);

		bgfx::updateDynamicIndexBuffer(
				mIndexBufferHandle,
				0,
				bgfx::copy(line_indices.data(), sizeof(uint16_t) * line_indices.size())
				);
	}

	assert (bgfx::isValid(mVertexBufferHandle));
	assert (bgfx::isValid(mIndexBufferHandle));
}

void Renderer::setupShaders() {
	// Create uniforms
	sceneDefaultTextureSampler = bgfx::createUniform("sceneDefaultTexture", bgfx::UniformType::Int1);
	u_mtx        = bgfx::createUniform("u_mtx",        bgfx::UniformType::Mat4);
	u_flags      = bgfx::createUniform("u_flags",      bgfx::UniformType::Vec4);
	u_camPos     = bgfx::createUniform("u_camPos",     bgfx::UniformType::Vec4);
	s_texCube    = bgfx::createUniform("s_texCube",    bgfx::UniformType::Int1);
	s_texCubeIrr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Int1);

	int grid_size = 1024;
	int grid_border = 12;
	uint8_t grid_color_border [4] = {32, 32, 32, 255};
//	uint8_t grid_color_border [4] = {0, 0, 0, 0};
	uint8_t grid_color_0[4] = {192, 192, 192, 255};
	uint8_t grid_color_1[4] = {128, 128, 128, 255};
	uint8_t* texture_data = NULL;
	texture_data = new uint8_t[grid_size * grid_size * 4];
	for (int i = 0; i < grid_size; i++) {
		for (int j = 0; j < grid_size; j++) {
			uint8_t *texel = &texture_data[i * (grid_size * 4) + j * 4];
			if ( (i < (grid_border / 2))
					|| (i > grid_size - (grid_border / 2))
					|| (j < (grid_border / 2))
					|| (j > grid_size - (grid_border / 2)) ) {
				memcpy (texel, grid_color_border, sizeof (uint8_t) * 4);
			}
			else {
				if ( (i * 2) / grid_size + (j * 2) / grid_size == 1) {
					memcpy (texel, grid_color_0, sizeof (uint8_t) * 4);
				} else {
					memcpy (texel, grid_color_1, sizeof (uint8_t) * 4);
				}
			}
		}
	}

	sceneDefaultTexture = bgfx::createTexture2D(grid_size, grid_size, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy (texture_data, grid_size * grid_size * 4));

	delete[] texture_data;

//	sceneDefaultTexture = bgfxutils::loadTexture("fieldstone-rgba.dds");

	u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
	u_line_params = bgfx::createUniform("u_line_params", bgfx::UniformType::Vec4);

	m_timeOffset = bx::getHPCounter();

	// Initialize light
	gLog ("Creating light uniforms...");
	lights[0].u_shadowMap = bgfx::createUniform("u_shadowMap", bgfx::UniformType::Int1);
	lights[0].u_shadowMapParams = bgfx::createUniform("u_shadowMapParams", bgfx::UniformType::Vec4);
	lights[0].u_lightPos  = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
	lights[0].u_lightMtx  = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Int1);

	// Setup the light probe pass
	IBL::uniforms.init();
	IBL::uniforms.m_glossiness   = IBL::settings.m_glossiness;
	IBL::uniforms.m_reflectivity = IBL::settings.m_reflectivity;
	IBL::uniforms.m_exposure     = IBL::settings.m_exposure;
	IBL::uniforms.m_bgType       = IBL::settings.m_bgType;
	IBL::uniforms.m_metalOrSpec   = float(IBL::settings.m_metalOrSpec);
	IBL::uniforms.m_doDiffuse     = float(IBL::settings.m_doDiffuse);
	IBL::uniforms.m_doSpecular    = float(IBL::settings.m_doSpecular);
	IBL::uniforms.m_doDiffuseIbl  = float(IBL::settings.m_doDiffuseIbl);
	IBL::uniforms.m_doSpecularIbl = float(IBL::settings.m_doSpecularIbl);
	memcpy(IBL::uniforms.m_rgbDiff,  IBL::settings.m_rgbDiff,  3*sizeof(float) );
	memcpy(IBL::uniforms.m_rgbSpec,  IBL::settings.m_rgbSpec,  3*sizeof(float) );
	memcpy(IBL::uniforms.m_lightDir, IBL::settings.m_lightDir, 3*sizeof(float) );
	memcpy(IBL::uniforms.m_lightCol, IBL::settings.m_lightCol, 3*sizeof(float) );

	s_renderStates[RenderState::Skybox].m_program = RenderProgram("shaders/src/vs_ibl_skybox.sc", "shaders/src/fs_ibl_skybox.sc");

	// Get renderer capabilities info.
	const bgfx::Caps* caps = bgfx::getCaps();
	// Shadow samplers are supported at least partially supported if texture
	// compare less equal feature is supported.
	bool shadowSamplerSupported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);

	if (shadowSamplerSupported)
	{
		// Depth textures and shadow samplers are supported.
		s_renderStates[RenderState::ShadowMap].m_program = RenderProgram("shaders/src/vs_sms_mesh.sc", "shaders/src/fs_sms_shadow.sc");
		s_renderStates[RenderState::Scene].m_program = RenderProgram("shaders/src/vs_sms_mesh.sc",   "shaders/src/fs_sms_mesh.sc");
		s_renderStates[RenderState::SceneTextured].m_program = RenderProgram("shaders/src/vs_sms_mesh_textured.sc",   "shaders/src/fs_sms_mesh_textured.sc");

		lights[0].shadowMapTexture= bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::D16, BGFX_TEXTURE_COMPARE_LEQUAL);
		bgfx::TextureHandle fbtextures[] = { lights[0].shadowMapTexture };
		lights[0].shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}
	else
	{
		// Depth textures and shadow samplers are not supported. Use float
		// depth packing into color buffer instead.
		s_renderStates[RenderState::ShadowMap].m_program.program = bgfxutils::loadProgram("vs_sms_shadow_pd", "fs_sms_shadow_pd");
		s_renderStates[RenderState::Scene].m_program.program = bgfxutils::loadProgram("vs_sms_mesh",      "fs_sms_mesh_pd");
		s_renderStates[RenderState::SceneTextured].m_program.program = bgfxutils::loadProgram("vs_sms_mesh_textured",      "fs_sms_mesh_pd_textured");

		lights[0].shadowMapTexture = bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT);
		bgfx::TextureHandle fbtextures[] =
		{
			lights[0].shadowMapTexture,
			bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::D16, BGFX_TEXTURE_RT_WRITE_ONLY),
		};
		lights[0].shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}

	s_renderStates[RenderState::Lines].m_program = RenderProgram("shaders/lines/vs_lines.sc", "shaders/lines/fs_lines.sc");

	s_renderStates[RenderState::LinesOccluded].m_program = RenderProgram("shaders/lines/vs_lines.sc", "shaders/lines/fs_lines_occluded.sc");

	s_renderStates[RenderState::Debug].m_program = RenderProgram("shaders/src/vs_debug.sc", "shaders/src/fs_debug.sc");
}

void Renderer::setupRenderPasses() {
	// ShadowMap
	s_renderStates[RenderState::ShadowMap].m_viewId = RenderState::ShadowMap;

	// Scene
	s_renderStates[RenderState::Scene].m_viewId  = RenderState::Scene;
	s_renderStates[RenderState::Scene].m_numTextures = 1;

	// Scene: shadow map texture
	s_renderStates[RenderState::Scene].m_textures[0].m_flags = UINT32_MAX;
	s_renderStates[RenderState::Scene].m_textures[0].m_stage = 0;
	s_renderStates[RenderState::Scene].m_textures[0].m_sampler = lights[0].u_shadowMap;
	s_renderStates[RenderState::Scene].m_textures[0].m_texture = lights[0].shadowMapTexture;

	// Scene: default texture
	s_renderStates[RenderState::SceneTextured].m_viewId  = RenderState::SceneTextured;
	s_renderStates[RenderState::SceneTextured].m_numTextures = 2;

	s_renderStates[RenderState::SceneTextured].m_textures[0].m_flags = UINT32_MAX;
	s_renderStates[RenderState::SceneTextured].m_textures[0].m_stage = 0;
	s_renderStates[RenderState::SceneTextured].m_textures[0].m_sampler = lights[0].u_shadowMap;
	s_renderStates[RenderState::SceneTextured].m_textures[0].m_texture = lights[0].shadowMapTexture;

	s_renderStates[RenderState::SceneTextured].m_textures[1].m_flags = UINT32_MAX;
	s_renderStates[RenderState::SceneTextured].m_textures[1].m_stage = 1;
	s_renderStates[RenderState::SceneTextured].m_textures[1].m_sampler = sceneDefaultTextureSampler;
	s_renderStates[RenderState::SceneTextured].m_textures[1].m_texture = sceneDefaultTexture;

	// Lines
	s_renderStates[RenderState::Lines].m_viewId = RenderState::Lines;

	// Lines
	s_renderStates[RenderState::LinesOccluded].m_viewId = RenderState::LinesOccluded;

	// Debug
	s_renderStates[RenderState::Debug].m_viewId = RenderState::Debug;
}

// void Renderer::setupWindowX11 (Display* x11_display, int x11_window_id) {
// 	bgfx::x11SetDisplayWindow(x11_display, x11_window_id);
// }

class BGFXCallbacks: public bgfx::CallbackI {
	virtual void fatal (bgfx::Fatal::Enum _code, const char *_str) {
		std::cerr << "Fatal (" << _code << "): " << _str << std::endl;
	}

	virtual void traceVargs (const char *_filePath, uint16_t _line, const char* _format, va_list _argList) {
		char output_buffer[255];
		vsprintf (output_buffer, _format, _argList);
		std::cerr << "Trace " << _filePath << ":" << _line << " : " << output_buffer;
	}

	virtual uint32_t cacheReadSize(uint64_t _id) {
		return 0;
	}

	virtual bool cacheRead(uint64_t _id, void *_data, uint32_t _size) {
		return false;
	}

	virtual void cacheWrite(uint64_t _id, const void *_data, uint32_t _size) {
	}

	virtual void screenShot(const char *_filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void *_data, uint32_t _size, bool _yflip) {
	}

	virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, bgfx::TextureFormat::Enum _format, bool _yflip) {
	}

	virtual void captureEnd() {
	};

	virtual void captureFrame(const void *_data, uint32_t _size) {
	};
};

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


void Renderer::initialize(int width, int height) {
	this->view_width = width;
	this->view_height = height;

	uint32_t debug = BGFX_DEBUG_TEXT;
	uint32_t reset = BGFX_RESET_VSYNC;

	reset = BGFX_RESET_VSYNC | BGFX_RESET_MAXANISOTROPY | BGFX_RESET_MSAA_X16;
	bgfx::reset(view_width, view_height, reset);

	bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);
	bgfx::setViewRect(0, view_offset_x, view_offset_y, view_width, view_height);

	bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	flipV = false
		|| renderer == bgfx::RendererType::OpenGL
		|| renderer == bgfx::RendererType::OpenGLES
		;

	bgfx::setDebug(debug);

	gLog ("Creating Cameras");
	cameras.push_back (Camera());
	activeCameraIndex = 0;
	lights.push_back (Light());

	createGeometries();

	setupShaders();

	setupRenderPasses();

	mLightProbes[LightProbe::Bolonga].load("bolonga");
	mLightProbes[LightProbe::Kyoto  ].load("kyoto");
	mCurrentLightProbe = LightProbe::Bolonga;

	initialized = true;
	resize (view_offset_x, view_offset_y, view_width, view_height);
	bgfx::frame();
}

void Renderer::shutdown() {
	bgfx::destroyIndexBuffer(cube_ibh);
	bgfx::destroyIndexBuffer(cube_edges_ibh);
	bgfx::destroyVertexBuffer(cube_vbh);
	bgfx::destroyIndexBuffer(plane_ibh);
	bgfx::destroyVertexBuffer(plane_vbh);

	bgfx::destroyUniform(u_camPos);
	bgfx::destroyUniform(u_flags);
	bgfx::destroyUniform(u_mtx);

	IBL::uniforms.destroy();

	bgfx::destroyUniform(s_texCube);
	bgfx::destroyUniform(s_texCubeIrr);

	bgfx::destroyUniform(u_time);
	bgfx::destroyUniform(u_color);
	bgfx::destroyUniform(u_line_params);

	for (uint8_t ii = 0; ii < RenderState::Count; ++ii) {
		if (bgfx::isValid(s_renderStates[ii].m_program.program)) {
			bgfx::destroyProgram(s_renderStates[ii].m_program.program);
		}
	}

	for (uint8_t ii = 0; ii < LightProbe::Count; ++ii) {
		mLightProbes[ii].destroy();
	}

	for (size_t i = 0; i < entities.size(); i++) {
		delete entities[i];
		entities[i] = NULL;
	}

	for (size_t i = 0; i < lights.size(); i++) {
		gLog ("Destroying light uniforms for light %d", i);
		bgfx::destroyFrameBuffer(lights[i].shadowMapFB);

		bgfx::destroyUniform(lights[i].u_shadowMap);
		bgfx::destroyUniform(lights[i].u_shadowMapParams);
		bgfx::destroyUniform(lights[i].u_lightPos);
		bgfx::destroyUniform(lights[i].u_lightMtx);
	}
	lights.clear();

	cameras.clear();
}

void Renderer::resize (int x, int y, int width, int height) {
	if (initialized) {
		this->view_offset_x = x;
		this->view_offset_y = y;
		this->view_width = width;
		this->view_height = height;

		for (uint32_t i = 0; i < cameras.size(); i++) {
			cameras[i].width = static_cast<float>(width);
			cameras[i].height = static_cast<float>(height);
		}

		ImGuizmo::SetRect(view_offset_x, view_offset_y, view_width, view_height);
	}
}

void Renderer::paintGL() {
	int64_t now = bx::getHPCounter();
	static int64_t last = now;
	const int64_t frameTime = now - last;
	last = now;
	const double freq = double(bx::getHPFrequency() );
	const double toMs = 1000.0/freq;

	float time = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );
	bgfx::setUniform (u_time, &time);

	// Use debug font to print information about this example.
	bgfx::dbgTextClear();

	// debug font is 8 pixels wide
	int num_chars = view_width / 8;
	bgfx::dbgTextPrintf(num_chars - 18, 0, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

	// This dummy draw call is here to make sure that view 0 is cleared
	// if no other draw calls are submitted to view 0.
	bgfx::touch(0);

	// update camera matrices
	for (uint32_t i = 0; i < cameras.size(); i++) {
		cameras[i].updateMatrices();
	}

	// lights: update view and projection matrices and shadow map parameters
	for (uint32_t i = 0; i < lights.size(); i++) {
		bgfx::setUniform(lights[i].u_lightPos, lights[i].pos.data());
		float shadow_map_params[4];
		shadow_map_params[0] = static_cast<float>(lights[i].shadowMapSize);
		shadow_map_params[1] = lights[0].shadowMapBias;
		shadow_map_params[2] = 0.f;
		shadow_map_params[3] = 0.f;
		bgfx::setUniform(lights[i].u_shadowMapParams, &shadow_map_params);

		float eye[3];
		eye[0] = lights[i].pos[0];
		eye[1] = lights[i].pos[1];
		eye[2] = lights[i].pos[2];

		float at[3];
		at[0] = - lights[i].pos[0] + lights[i].dir[0];
		at[1] = - lights[i].pos[1] + lights[i].dir[1];
		at[2] = - lights[i].pos[2] + lights[i].dir[2];

		bx::mtxLookAtRh(lights[i].mtxView, eye, at);

		lights[i].area = 10.0f;
		lights[i].near = 10.f;
		lights[i].far = 20.f;

		//	bx::mtxProj(lightProj, 20.0f, 1., 5.f, 10.0f);

		bx::mtxOrthoRh(lights[i].mtxProj, 
				lights[i].area, -lights[i].area, 
				lights[i].area, -lights[i].area, 
				lights[i].near, lights[i].far);

		// lights: shadow matrix
		const float sy = flipV ? 0.5f : -0.5f;
		const float mtxCrop[16] =
		{
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f,   sy, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f,
		};

		float mtxTmp[16];
		bx::mtxMul(mtxTmp,    lights[i].mtxProj, mtxCrop);
		bx::mtxMul(lights[i].mtxShadow, lights[i].mtxView, mtxTmp);
	}

	// setup render passes
	float view[16];
	float proj[16];
	bx::mtxIdentity(view);
	bx::mtxOrthoRh(proj, 0.f, 1.f, 1.f, 0.f, 0.f, 100.0f);

	bgfx::setViewRect(RenderState::Skybox, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::Skybox, view, proj);
	
	bgfx::setViewRect(RenderState::ShadowMap, 0, 0, lights[0].shadowMapSize, lights[0].shadowMapSize);
	bgfx::setViewFrameBuffer(RenderState::ShadowMap, lights[0].shadowMapFB);
	bgfx::setViewTransform(RenderState::ShadowMap, lights[0].mtxView, lights[0].mtxProj);

	bgfx::setViewRect(RenderState::Scene, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::Scene, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::SceneTextured, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::SceneTextured, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::Lines, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::Lines, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::LinesOccluded, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::LinesOccluded, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::Debug, view_offset_x, view_offset_y, view_width, view_height);
	bgfx::setViewTransform(RenderState::Debug, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);


	//
	// Pass: floor
	//

	// setup floor
	float mtxFloor[16];
	bx::mtxSRT(mtxFloor
			, 10.0f, 10.0f, 10.0f
			, 0.0f, 0.0f, 0.0f
			, 0.0f, -0.009f, 0.0f
			);

	float lightMtx[16];
	// Floor.
	bx::mtxMul(lightMtx, mtxFloor, lights[0].mtxShadow);
	bgfx::setUniform(lights[0].u_lightMtx, lightMtx);

	// Clear backbuffer and shadowmap framebuffer at beginning.
	bgfx::setViewClear(RenderState::Skybox
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x000000ff, 1.0f, 0
			);

	bgfx::setViewClear(RenderState::ShadowMap
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff, 1.0f, 0
			);

//	bgfx::setViewClear(RenderState::Scene
//			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
//			, 0x303030ff, 1.0f, 0
//			);

	bgfx::touch(RenderState::Scene);
	bgfx::touch(RenderState::Skybox);

	//
	// Pass: skybox
	//

	if (drawSkybox) {
		// Skybox pass
		memcpy (IBL::uniforms.m_cameraPos, cameras[activeCameraIndex].eye.data(), 3 * sizeof(float));

		const float amount = bx::fmin(0.012/0.12f, 1.0f);
		IBL::settings.m_envRotCurr = bx::flerp(IBL::settings.m_envRotCurr, IBL::settings.m_envRotDest, amount);


		float mtxEnvRot[16];
		float env_rot_cur = 0.0f;
		float mtx_u_mtx[16];

		bx::mtxRotateY(mtxEnvRot, env_rot_cur);
		bx::mtxMul(IBL::uniforms.m_mtx, cameras[activeCameraIndex].mtxEnv, mtxEnvRot); // Used for Skybox.

		bgfx::setTexture(0, s_texCube, mLightProbes[mCurrentLightProbe].m_tex);
		bgfx::setTexture(1, s_texCubeIrr, mLightProbes[mCurrentLightProbe].m_texIrr);
		bgfx::setState(BGFX_STATE_RGB_WRITE|BGFX_STATE_ALPHA_WRITE);
		screenSpaceQuad( 
				(float)cameras[activeCameraIndex].width,
				(float)cameras[activeCameraIndex].height, true);

		IBL::uniforms.submit();
		bgfx::submit(RenderState::Skybox, s_renderStates[RenderState::Skybox].m_program.program);
	}

	if (drawFloor)
	{
		// render the plane
		uint32_t cached = bgfx::setTransform(mtxFloor);
		for (uint32_t pass = 0; pass < RenderState::Count; ++pass) {
			// Only draw plane textured or during the shadow map passes
			if (pass != RenderState::SceneTextured
					&& pass != RenderState::ShadowMap)
				continue;

			const RenderState& st = s_renderStates[pass];
			if (!isValid(st.m_program.program)) {
				continue;
			}

			bgfx::setTransform(cached);
			for (uint8_t tex = 0; tex < st.m_numTextures; ++tex)
			{
				const RenderState::Texture& texture = st.m_textures[tex];
				bgfx::setTexture(texture.m_stage
						, texture.m_sampler
						, texture.m_texture
						, texture.m_flags
						);
			}

			bgfx::setUniform(lights[0].u_lightMtx, lightMtx);
			bgfx::setUniform(u_color, Vector4f(1.f, 1.f, 1.f, 1.f).data(), 4);
			bgfx::setIndexBuffer(plane_ibh);
			bgfx::setVertexBuffer(plane_vbh);
			bgfx::setState(st.m_state);
			bgfx::submit(st.m_viewId, st.m_program.program);
		}
	}

	//
	// Pass: shadow map and scene pass
	//
		
	// render entities
	for (size_t i = 0; i < entities.size(); i++) {
		if (entities[i]->mDrawEntity == false)
			continue;

		float mtxLightViewProjInv[16];
		float light_pos_world[3];
		
		// shadow map pass
		for (uint32_t j = 0; j < entities[i]->mSkeletonMeshes.Length(); ++j) {
			bx::mtxMul(
					lightMtx, 
					entities[i]->mSkeletonMeshes.GetBoneMatrix(j).data(),
					lights[0].mtxShadow
					);
			bgfx::setUniform(lights[0].u_lightMtx, lightMtx);
			entities[i]->mSkeletonMeshes.GetMesh(j)->Submit(
					&s_renderStates[RenderState::ShadowMap],
					entities[i]->mSkeletonMeshes.GetBoneMatrix(j).data()
					);
		}

		// scene pass
		for (uint32_t j = 0; j < entities[i]->mSkeletonMeshes.Length(); ++j) {
			bx::mtxMul(
					lightMtx, 
					entities[i]->mSkeletonMeshes.GetBoneMatrix(j).data(),
					lights[0].mtxShadow
					);

			// compute world position of the light
			Vector4f light_pos4 (
					lights[0].pos[0],
					lights[0].pos[1],
					lights[0].pos[2],
					1.0f
					);
			Vector4f light_pos = 
				entities[i]->mSkeletonMeshes.GetBoneMatrix(j)
				* light_pos4;

			bgfx::setUniform(lights[0].u_lightPos, light_pos.data());
			bgfx::setUniform(u_color, entities[i]->mColor.data());
			bgfx::setUniform(lights[0].u_lightMtx, lightMtx);
			entities[i]->mSkeletonMeshes.GetMesh(j)->Submit(
					&s_renderStates[RenderState::Scene],
					entities[i]->mSkeletonMeshes.GetBoneMatrix(j).data()
					);
		}

	}

	// render debug information
	if (drawDebug) {
		float tmp[16];

		// render light frustums 
		for (uint32_t i = 0; i < lights.size(); i++) {
			bx::mtxMul (tmp, lights[i].mtxView, lights[i].mtxProj);

			float mtxLightViewProjInv[16];
			bx::mtxInverse (mtxLightViewProjInv, tmp);
			bgfx::setUniform(u_color, Vector4f(1.f, 1.f, 0.3f, 1.0f).data(), 4);

			const RenderState& st = s_renderStates[RenderState::Debug];
			bgfx::setTransform(mtxLightViewProjInv);

			bgfx::setIndexBuffer(cube_edges_ibh);
			bgfx::setVertexBuffer(cube_vbh);
			bgfx::setState(st.m_state);
			bgfx::submit(st.m_viewId, st.m_program.program);
		}

		// render camera frustums 
		for (uint32_t i = 0; i < cameras.size(); i++) {
			bx::mtxMul (tmp, cameras[i].mtxView, cameras[i].mtxProj);

			float mtxCameraViewProjInv[16];
			bx::mtxInverse (mtxCameraViewProjInv, tmp);
			bgfx::setUniform(u_color, Vector4f(0.5f, 0.5f, 0.8f, 1.f).data(), 4);

			const RenderState& st = s_renderStates[RenderState::Debug];
			bgfx::setTransform(mtxCameraViewProjInv);

			bgfx::setIndexBuffer(cube_edges_ibh);
			bgfx::setVertexBuffer(cube_vbh);
			bgfx::setState(st.m_state);
			bgfx::submit(st.m_viewId, st.m_program.program);
		}

		// render debug skeletons
		for (size_t i = 0; i < entities.size(); i++) {
			debugBoneMesh.Submit(&s_renderStates[RenderState::Scene],
					entities[i]->mTransform.toMatrix().data());
		}

		// debug commands
		bgfx::setUniform(u_color, Vector4f(1.0f, 1.0f, 1.0f, 1.f).data(), 4);

		// assemble lines for alls debug lines
		debugPaths.resize(debugCommands.size());
		for (uint32_t i = 0; i < debugCommands.size(); i++) {
			Path &line = debugPaths[i];

			if (debugCommands[i].type == DebugCommand::Line) {
				line.points.clear();
				line.colors.clear();

				line.points.push_back(debugCommands[i].from);
				line.colors.push_back(debugCommands[i].color);

				line.points.push_back(debugCommands[i].to);
				line.colors.push_back(debugCommands[i].color);

				line.UpdateBuffers();
			} else if (debugCommands[i].type == DebugCommand::Circle) {
				line.points.clear();
				line.colors.clear();
				float radius = debugCommands[i].radius;
				float c,s,angle;

				// construct an orthogonal vector from the normal.
				Vector3f plane1 (
						debugCommands[i].to[1] - debugCommands[i].to[2],
						debugCommands[i].to[0],
						-debugCommands[i].to[0]
						);
				plane1.normalize();
				Vector3f plane2 = debugCommands[i].to.cross(plane1);

				const int cNumSegments = 64;
				for (uint32_t j = 0; j < cNumSegments; j++) {
					angle = M_PI * 0.5f + 2. * M_PI * static_cast<float>(j) / static_cast<float>(cNumSegments - 1);

					s = sin(angle) * radius;
					c = cos(angle) * radius;

					line.points.push_back(debugCommands[i].from + plane1 * s + plane2 * c);
					line.colors.push_back(debugCommands[i].color);
				}

				line.UpdateBuffers();
			}

			float thickness = 0.05143f;
			float miter = 0.0f;
			float aspect = static_cast<float>(view_width) / static_cast<float>(view_height);

			Vector4f params (thickness, miter, aspect, 0.0f);

			// submit data to regular lines state
			const RenderState& st = s_renderStates[RenderState::Lines];

			bgfx::setUniform(u_line_params, params.data(), 1);
			bgfx::setIndexBuffer(line.mIndexBufferHandle);
			bgfx::setVertexBuffer(line.mVertexBufferHandle);
			bgfx::setState(st.m_state);
			bgfx::submit(st.m_viewId, st.m_program.program);

			// submit data to state LinesOccluded
			bgfx::setState(s_renderStates[RenderState::LinesOccluded].m_state);
			bgfx::setUniform(u_line_params, params.data(), 1);
			bgfx::setIndexBuffer(line.mIndexBufferHandle);
			bgfx::setVertexBuffer(line.mVertexBufferHandle);
				bgfx::submit(
					s_renderStates[RenderState::LinesOccluded].m_viewId,
					s_renderStates[RenderState::LinesOccluded].m_program.program
					);
		}
	}

	// Advance to next frame. Rendering thread will be kicked to
	// process submitted rendering primitives.
	bgfx::frame();

//	ImGui::SetNextWindowSize (ImVec2(400.f, 300.0f), ImGuiSetCond_Once);
//	ImGui::SetNextWindowPos (ImVec2(10.f, 300.0f), ImGuiSetCond_Once);
//
	if (ImGui::BeginDock("Render Settings")) {
		if(ImGui::DragFloat3 ("Light0 Pos", lights[0].pos.data(), 1.0f, -10.0f, 10.0f)) {
		}

		if(ImGui::DragFloat3 ("Light0 Dir", lights[0].dir.data(), 1.0f, -10.0f, 10.0f)) {
		}

		float light_at[3];
		light_at[0] = lights[0].dir[0] - lights[0].pos[0];
		light_at[1] = lights[0].dir[1] - lights[0].pos[1];
		light_at[2] = lights[0].dir[2] - lights[0].pos[2];

		if(ImGui::DragFloat3 ("Light0 At", light_at, 1.0f, -10.0f, 10.0f)) {
			lights[0].dir[0] =  lights[0].pos[0]- light_at[0];
			lights[0].dir[1] =  lights[0].pos[1]- light_at[1];
			lights[0].dir[2] =  lights[0].pos[2]- light_at[2];
		}

		assert (lights.size() == 1);

		ImGui::Checkbox("Draw Floor", &drawFloor);
		ImGui::Checkbox("Draw Skybox", &drawSkybox);
		ImGui::Checkbox("Draw Debug", &drawDebug);

		for (int i = 0; i < lights.size(); i++) {
			ImGui::SliderFloat("Bias", 
					&lights[i].shadowMapBias,
					0.0001f,
					0.10f
					);
		}
	}

	ImGui::EndDock();

	// clear debug commands as they have to be issued every frame
	debugCommands.clear();
}

bool Renderer::updateShaders() {
	bool result = true;
	for (int i = 0; i < RenderState::Count; i++) {
		RenderState& st = s_renderStates[i];

		if (st.m_program.vertexShaderFileName != ""
				&& st.m_program.fragmentShaderFileName!= ""
			 ) {
			if (st.m_program.checkModified()) {
				bool load_success = st.m_program.reload();

				// if so far everything was successful but this one failed
				// make sure to set the return value to false
				if (result == true && ! load_success)
					result = false;
			}
		}
	}

	return true;
}

Entity* Renderer::createEntity() {
	Entity* result = new Entity();
	entities.push_back(result);

	return result;
}

bool Renderer::destroyEntity(Entity* entity) {
	int i = 0;
	for (i = 0; i < entities.size(); i++) {
		if (entities[i] == entity) {
			break;
		}
	}

	if (i != entities.size()) {
		delete entity;

		entities.erase(entities.begin() + i);

		return true;
	}

	return false;
}

// debug commands
void Renderer::drawDebugLine (
		const Vector3f &from,
		const Vector3f &to,
		const Vector3f &color) {
	DebugCommand cmd;

	cmd.type = DebugCommand::Line;
	cmd.from = from;
	cmd.to = to;
	cmd.color = Vector4f (color[0], color[1], color[2], 1.0f);

	debugCommands.push_back(cmd);
}

void Renderer::drawDebugAxes (
		const Vector3f &pos,
		const Matrix33f &orientation,
		const float &scale) {

	drawDebugLine (pos, pos + Vector3f (orientation.block<3,1>(0,0)) * scale, Vector3f (1.f, 0.f, 0.f));
	drawDebugLine (pos, pos + Vector3f (orientation.block<3,1>(0,1)) * scale, Vector3f (0.f, 1.f, 0.f));
	drawDebugLine (pos, pos + Vector3f (orientation.block<3,1>(0,2)) * scale, Vector3f (0.f, 0.f, 1.f));
}

void Renderer::drawDebugCircle (
		const Vector3f &pos,
		const Vector3f &normal,
		const float radius,
		const Vector4f &color) {
	DebugCommand cmd;
	cmd.type = DebugCommand::Circle;
	cmd.from = pos;
	cmd.to = normal;
	cmd.color = color;
	cmd.radius = radius;

	debugCommands.push_back(cmd);
}

void Renderer::drawDebugSphere (
		const Vector3f &pos,
		float radius,
		const Vector4f &color) {
	drawDebugCircle (
			pos, 
			Vector3f (1.0f, 0.0f, 0.0f),
			radius,
			color);
	drawDebugCircle (
			pos, 
			Vector3f (0.0f, 1.0f, 0.0f),
			radius,
			color);
	drawDebugCircle (
			pos, 
			Vector3f (0.0f, 0.0f, 1.0f),
			radius,
			color);
}


