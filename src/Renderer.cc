#include "Renderer.h"

#include <string>
#include <iostream>

#include <assert.h>

#include <bgfx/bgfxplatform.h>
#include <bx/thread.h>

#include <bx/timer.h>
#include <bx/fpumath.h>
#include <bx/uint32_t.h>
#include <dbg.h>

#include "imgui/imgui.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math_types.h"

bgfx::VertexBufferHandle cube_vbh;
bgfx::IndexBufferHandle cube_ibh;
bgfx::IndexBufferHandle cube_edges_ibh;
bgfx::VertexBufferHandle plane_vbh;
bgfx::IndexBufferHandle plane_ibh;
bgfx::UniformHandle u_time;
bgfx::UniformHandle u_color;
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
// Render states
//

static RenderState s_renderStates[RenderState::Count] = {
	{ // ShadowMap
   	0 
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_MSAA,
		0,
		UINT16_MAX,
		RenderState::ShadowMap
	},
	{ // Scene
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_MSAA,
		0,
		UINT16_MAX,
		RenderState::Scene
	},
	{ // SceneTextured
		0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_MSAA,
		0,
		UINT16_MAX,
		RenderState::SceneTextured
	},
	{ // Debug
		0	
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_PT_LINES
		| BGFX_STATE_MSAA,
		0,
		UINT16_MAX,
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

// 
// Static geometries
// 

// Plane
static PosNormalColorTexcoordVertex s_hplaneVertices[] =
{
	{ -1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f),      0.f,      0.f },
	{  1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f), 10.f,      0.f },
	{ -1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f),     0.f,  10.f},
	{  1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(1.0f, 1.0f, 1.0f), 10.f,  10.f },
};

static const uint16_t s_planeIndices[] =
{
	0, 1, 2,
	1, 3, 2,
};

// Cube
static PosColorVertex s_cubeVertices[8] =
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

static const uint16_t s_cubeEdgeIndices[24] =
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

static const uint16_t s_cubeIndices[36] =
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

void Camera::updateMatrices() {
	assert (width != -1.f && height != -1.f);
	float aspect = width / height;

	bx::mtxLookAt (mtxView, eye, poi, up);

	if (orthographic) {
		bx::mtxOrtho(mtxProj, 
				-width * 0.5f, width * 0.5f,
				-height * 0.5f, height * 0.5f, 
				near, far);
	} else {
		bx::mtxProj(mtxProj, fov, aspect, near, far);
	}
}

void Renderer::createGeometries() {
	// Create vertex stream declaration.
	PosColorVertex::init();
	PosNormalVertex::init();
	PosNormalColorTexcoordVertex::init();

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
}

void Renderer::setupShaders() {
	// Create uniforms
	sceneDefaultTextureSampler = bgfx::createUniform("sceneDefaultTexture", bgfx::UniformType::Int1);

	int grid_size = 1024;
	int grid_border = 12;
	uint8_t grid_color_border [4] = {255, 255, 255, 255};
	uint8_t grid_color_0[4] = {192, 192, 192, 255};
	uint8_t grid_color_1[4] = {96, 96, 96, 255};
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

	m_timeOffset = bx::getHPCounter();

	// Initialize light
	lights[0].u_shadowMap = bgfx::createUniform("u_shadowMap", bgfx::UniformType::Int1);
	lights[0].u_shadowMapParams = bgfx::createUniform("u_shadowMapParams", bgfx::UniformType::Vec4);
	lights[0].u_lightPos  = bgfx::createUniform("u_lightPos", bgfx::UniformType::Int1);
	lights[0].u_lightMtx  = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Int1);

	// Get renderer capabilities info.
	const bgfx::Caps* caps = bgfx::getCaps();
	// Shadow samplers are supported at least partially supported if texture
	// compare less equal feature is supported.
	bool shadowSamplerSupported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);

	if (shadowSamplerSupported)
	{
		// Depth textures and shadow samplers are supported.
		s_renderStates[RenderState::ShadowMap].m_program = bgfxutils::loadProgramFromFiles("shaders/src/vs_sms_mesh.sc", "shaders/src/fs_sms_shadow.sc");
		s_renderStates[RenderState::Scene].m_program = bgfxutils::loadProgramFromFiles("shaders/src/vs_sms_mesh.sc",   "shaders/src/fs_sms_mesh.sc");
		s_renderStates[RenderState::SceneTextured].m_program = bgfxutils::loadProgramFromFiles("shaders/src/vs_sms_mesh_textured.sc",   "shaders/src/fs_sms_mesh_textured.sc");

		lights[0].shadowMapTexture= bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::D16, BGFX_TEXTURE_COMPARE_LEQUAL);
		bgfx::TextureHandle fbtextures[] = { lights[0].shadowMapTexture };
		lights[0].shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}
	else
	{
		// Depth textures and shadow samplers are not supported. Use float
		// depth packing into color buffer instead.
		s_renderStates[RenderState::ShadowMap].m_program = bgfxutils::loadProgram("vs_sms_shadow_pd", "fs_sms_shadow_pd");
		s_renderStates[RenderState::Scene].m_program = bgfxutils::loadProgram("vs_sms_mesh",      "fs_sms_mesh_pd");
		s_renderStates[RenderState::SceneTextured].m_program = bgfxutils::loadProgram("vs_sms_mesh_textured",      "fs_sms_mesh_pd_textured");

		lights[0].shadowMapTexture = bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT);
		bgfx::TextureHandle fbtextures[] =
		{
			lights[0].shadowMapTexture,
			bgfx::createTexture2D(lights[0].shadowMapSize, lights[0].shadowMapSize, false, 1, bgfx::TextureFormat::D16, BGFX_TEXTURE_RT_WRITE_ONLY),
		};
		lights[0].shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	}

	s_renderStates[RenderState::Debug].m_program = bgfxutils::loadProgramFromFiles("shaders/src/vs_debug.sc", "shaders/src/fs_debug.sc");
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

void Renderer::initialize(int width, int height) {
	this->width = width;
	this->height = height;

	uint32_t debug = BGFX_DEBUG_TEXT;
	uint32_t reset = BGFX_RESET_VSYNC;

	reset = BGFX_RESET_VSYNC | BGFX_RESET_MAXANISOTROPY | BGFX_RESET_MSAA_X16;

	bool result = bgfx::init();
	if (!result) {
		std::cerr << "Error: could not initialize renderer!" << std::endl;
		exit (EXIT_FAILURE);
	}

	bgfx::reset(width, height, reset);

	bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);
	bgfx::setViewRect(0, 0, 0, width, height);

	bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	flipV = false
		|| renderer == bgfx::RendererType::OpenGL
		|| renderer == bgfx::RendererType::OpenGLES
		;

	bgfx::setDebug(debug);

	cameras.push_back (Camera());
	activeCameraIndex = 0;
	lights.push_back (Light());

	// set the clear state
//	for (int i = 0; i < 2; i++) {
//		bgfx::setViewClear(i
//				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
//				, 0x303030ff
//				, 1.0f
//				, 0
//				);
//	}

	createGeometries();

	setupShaders();

	setupRenderPasses();

	// imgui initialization.
	imguiCreate();

//	// Start the imgui frame such that widgets can be submitted
//	imguiBeginFrame (inputState.mouseX,
//			inputState.mouseY,
//			inputState.mouseButton,
//			inputState.mouseScroll,
//			width,
//			height);
	
	initialized = true;
	resize (width, height);
	bgfx::frame();
}

void Renderer::shutdown() {
	imguiDestroy();

	bgfx::destroyFrameBuffer(lights[0].shadowMapFB);

	bgfx::destroyUniform(lights[0].u_shadowMap);
	bgfx::destroyUniform(lights[0].u_shadowMapParams);
	bgfx::destroyUniform(lights[0].u_lightPos);
	bgfx::destroyUniform(lights[0].u_lightMtx);

	bgfx::destroyIndexBuffer(cube_ibh);
	bgfx::destroyIndexBuffer(cube_edges_ibh);
	bgfx::destroyVertexBuffer(cube_vbh);
	bgfx::destroyIndexBuffer(plane_ibh);
	bgfx::destroyVertexBuffer(plane_vbh);

	bgfx::destroyUniform(u_time);
	bgfx::destroyUniform(u_color);

	for (size_t i = 0; i < entities.size(); i++) {
		delete entities[i];
		entities[i] = NULL;
	}

	for (size_t i = 0; i < meshes.size(); i++) {
		bgfxutils::meshUnload(meshes[i]);
		meshes[i] = NULL;
	}

	bgfx::shutdown();
}

void Renderer::resize (int width, int height) {
	if (initialized) {
		bgfx::reset (width, height);

		this->width = width;
		this->height = height;

		for (uint32_t i = 0; i < cameras.size(); i++) {
			cameras[i].width = static_cast<float>(width);
			cameras[i].height = static_cast<float>(height);
		}
	}
}

void Renderer::paintGLSimple() {
	// Set view 0 default viewport.
	bgfx::setViewRect(0, 0, 0, width, height);

	// This dummy draw call is here to make sure that view 0 is cleared
	// if no other draw calls are submitted to view 0.
	bgfx::touch(0);

	int64_t now = bx::getHPCounter();
	static int64_t last = now;
	const int64_t frameTime = now - last;
	last = now;
	const double freq = double(bx::getHPFrequency() );
	const double toMs = 1000.0/freq;

	// Use debug font to print information about this example.
	bgfx::dbgTextClear();
	bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/00-helloworld");
	bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Initialization and debug text.");
	bgfx::dbgTextPrintf(0, 3, 0x8f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

	// Advance to next frame. Rendering thread will be kicked to
	// process submitted rendering primitives.
	bgfx::frame();
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
	bgfx::dbgTextPrintf(0, 0, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

	// submit the imgui widgets
//	imguiEndFrame();

	// This dummy draw call is here to make sure that view 0 is cleared
	// if no other draw calls are submitted to view 0.
//	bgfx::touch(0);

	// update camera matrices
	for (uint32_t i = 0; i < cameras.size(); i++) {
		cameras[i].updateMatrices();
	}

	// lights: update view and projection matrices and shadow map parameters
	for (uint32_t i = 0; i < lights.size(); i++) {
		bgfx::setUniform(lights[i].u_lightPos, lights[i].pos);
		float shadow_map_params[4];
		shadow_map_params[0] = static_cast<float>(lights[i].shadowMapSize);
		shadow_map_params[1] = lights[0].shadowMapBias;
		shadow_map_params[2] = 0.f;
		shadow_map_params[3] = 0.f;
		bgfx::setUniform(lights[i].u_shadowMapParams, &shadow_map_params);

		float eye[3];
		eye[0] = -lights[i].pos[0];
		eye[1] = -lights[i].pos[1];
		eye[2] = -lights[0].pos[2];

		float at[3];
		at[0] = - lights[i].pos[0] + lights[i].dir[0];
		at[1] = - lights[i].pos[1] + lights[i].dir[1];
		at[2] = - lights[i].pos[2] + lights[i].dir[2];

		bx::mtxLookAt(lights[i].mtxView, eye, at);

		lights[i].area = 2.5f;
		lights[i].near = 0.f;
		lights[i].far = 5.f;

		//	bx::mtxProj(lightProj, 20.0f, 1., 5.f, 10.0f);

		bx::mtxOrtho(lights[i].mtxProj, -lights[i].area, lights[i].area, -lights[i].area, lights[i].area, lights[i].near, lights[i].far);

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
	bgfx::setViewRect(RenderState::ShadowMap, 0, 0, lights[0].shadowMapSize, lights[0].shadowMapSize);
	bgfx::setViewFrameBuffer(RenderState::ShadowMap, lights[0].shadowMapFB);
	bgfx::setViewTransform(RenderState::ShadowMap, lights[0].mtxView, lights[0].mtxProj);

	bgfx::setViewRect(RenderState::Scene, 0, 0, width, height);
	bgfx::setViewTransform(RenderState::Scene, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::SceneTextured, 0, 0, width, height);
	bgfx::setViewTransform(RenderState::SceneTextured, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	bgfx::setViewRect(RenderState::Debug, 0, 0, width, height);
	bgfx::setViewTransform(RenderState::Debug, cameras[activeCameraIndex].mtxView, cameras[activeCameraIndex].mtxProj);

	// setup floor
	float mtxFloor[16];
	bx::mtxSRT(mtxFloor
			, 10.0f, 10.0f, 10.0f
			, 0.0f, 0.0f, 0.0f
			, 0.0f, 0.0f, 0.0f
			);

	float lightMtx[16];
	// Floor.
	bx::mtxMul(lightMtx, mtxFloor, lights[0].mtxShadow);
	bgfx::setUniform(lights[0].u_lightMtx, lightMtx);

	// Clear backbuffer and shadowmap framebuffer at beginning.
	bgfx::setViewClear(RenderState::ShadowMap
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff, 1.0f, 0
			);

	bgfx::setViewClear(RenderState::Scene
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff, 1.0f, 0
			);

	bgfx::touch(RenderState::Scene);

	// render the plane
	uint32_t cached = bgfx::setTransform(mtxFloor);
	for (uint32_t pass = 0; pass < RenderState::Count; ++pass) {
		// Only draw plane textured or during the shadow map passes
		if (pass != RenderState::SceneTextured
				&& pass != RenderState::ShadowMap)
			continue;

		const RenderState& st = s_renderStates[pass];
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
		bgfx::submit(st.m_viewId, st.m_program);
	}

	// render entities
	for (size_t i = 0; i < entities.size(); i++) {
		// shadow map pass
		bx::mtxMul(lightMtx, entities[i]->transform, lights[0].mtxShadow);
		bgfx::setUniform(lights[0].u_lightMtx, lightMtx);
		bgfx::setUniform(u_color, entities[i]->color, 4);
		meshSubmit(entities[i]->mesh, &s_renderStates[RenderState::ShadowMap], 1, entities[i]->transform);

		// scene pass
		bx::mtxMul(lightMtx, entities[i]->transform, lights[0].mtxShadow);
		bgfx::setUniform(lights[0].u_lightMtx, lightMtx);
		bgfx::setUniform(u_color, entities[i]->color, 4);
		meshSubmit(entities[i]->mesh, &s_renderStates[RenderState::Scene], 1, entities[i]->transform);
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
			bgfx::submit(st.m_viewId, st.m_program);
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
			bgfx::submit(st.m_viewId, st.m_program);
		}

	}

	// Advance to next frame. Rendering thread will be kicked to
	// process submitted rendering primitives.
	bgfx::frame();

//	// Start the next imgui frame
//	imguiBeginFrame (inputState.mouseX,
//			inputState.mouseY,
//			inputState.mouseButton,
//			inputState.mouseScroll,
//			width,
//			height);
}

Entity* Renderer::createEntity() {
	Entity* result = new Entity();
	entities.push_back(result);

	return result;
}

bgfxutils::Mesh* Renderer::loadMesh(const char* filename) {
	MeshIdMap::iterator mesh_iter = meshIdMap.find (filename);
	bgfxutils::Mesh* result = NULL;

	if (mesh_iter == meshIdMap.end()) {
		std::string filename_str (filename);
		if (filename_str.substr(filename_str.size() - 4, 4) == ".obj") {
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			std::string err;
			bool result = tinyobj::LoadObj(shapes, materials, err, filename);

			if (!result) {
				std::cerr << "Error loading '" << filename << "': " << err << std::endl;
				exit(-1);
			}
//			result = bgfxutils::createMeshFromVBO (vbo);
		} else {
			result = bgfxutils::meshLoad(filename);
		}
		meshes.push_back (result);
		meshIdMap[filename] = meshes.size() - 1;
	} else {
		result = meshes[mesh_iter->second];
	}

	return result;
}
