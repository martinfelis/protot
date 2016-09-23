#pragma once

#include <cstdint>

#include <map>
#include <vector>

#include <bgfx/bgfx.h>

#include "RenderUtils.h"

struct Entity;

struct InputState {
	int32_t mousedX;
	int32_t mousedY;
	int32_t mouseX;
	int32_t mouseY;
	uint8_t mouseButton;
	int32_t mouseScroll;
	char key;

	InputState() :
		mouseX(0),
		mouseY(0),
		mouseButton(0),
		mouseScroll(0),
		key(0) {
		}
};

struct Camera {
	float eye[3];
	float poi[3];
	float up[3];

	float near;
	float far;
	float fov;
	bool orthographic;
	float width;
	float height;

	float mtxProj[16];
	float mtxView[16];

	Camera() :
		eye {5.f, 4.f, 5.f},
		poi {0.f, 2.f, 0.f},
		up  {0.f, 1.f, 0.f},
		near (1.f),
		far (20.f),
		fov (70.f),
		orthographic (false),
		width (-1.f),
		height (-1.f),

		mtxProj {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f},
		mtxView {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f}
	{}

	void updateMatrices();
};

struct Light {
	bgfx::UniformHandle u_shadowMap;
	bgfx::UniformHandle u_shadowMapParams;
	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightMtx;

	bgfx::TextureHandle shadowMapTexture;
	bgfx::FrameBufferHandle shadowMapFB;
	float pos[3];
	float dir[3];

	float mtxView[16];
	float mtxProj[16];
	float mtxLight[16];
	float mtxShadow[16];

	float shadowMapBias;
	uint16_t shadowMapSize;

	bool enabled;
	float near;
	float far;
	float area;

	Light() :
		u_shadowMap (BGFX_INVALID_HANDLE),
		u_lightPos (BGFX_INVALID_HANDLE),
		u_lightMtx (BGFX_INVALID_HANDLE),
		shadowMapTexture (BGFX_INVALID_HANDLE),
		shadowMapFB (BGFX_INVALID_HANDLE),
		pos {1.f, 1.f, 1.f},
		dir {-1.f, -1.f, -1.f},
		mtxView {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		},
		mtxProj {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		},
		mtxShadow {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		},
		shadowMapBias (0.0001f),
		shadowMapSize (1024),
		near (-100.f),
		far (100.f),
		area (10.f),
		enabled (false)
	{
	}
};

struct Entity {
	float transform[16];
	float color[4];
	bgfxutils::Mesh* mesh;

	Entity() :
		transform {
				1.f, 0.f, 0.f, 0.f,
				0.f, 1.f, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				0.f, 0.0, 0.f, 1.f
		},
		color { 1.f, 1.f, 1.f, 1.f },
		mesh (NULL) {};
};

struct Renderer {
	bool initialized;
	bool drawDebug;
	uint32_t width;
	uint32_t height;

	bgfx::UniformHandle sceneDefaultTextureSampler;
	bgfx::TextureHandle sceneDefaultTexture;

	std::vector<bgfxutils::Mesh*> meshes;
	typedef std::map<std::string, unsigned int> MeshIdMap;
	MeshIdMap meshIdMap;

	std::vector<Entity*> entities;

	std::vector<Camera> cameras;
	std::vector<Light> lights;

	uint16_t activeCameraIndex;

	// needed to forward inputs to IMGUI
	InputState inputState;

	Renderer() :
		initialized(false),
		drawDebug(false),
		width (0),
		height (0)
	{}

	// initialize simple geometries (cube, sphere, ...)
	void createGeometries();
	// create uniforms, load shaders, and create render targets
	void setupShaders();
	// setup renderpasses and wire up render targets
	void setupRenderPasses();

	void initialize(int width, int height);
	void shutdown();
	void paintGL();
	void paintGLSimple();
	void resize (int width, int height);

	Entity* createEntity();
	bgfxutils::Mesh* loadMesh(const char* filename);
};

struct RenderState {
	enum  {
		ShadowMap,
		Scene,
		SceneTextured,
		Debug,
		Count
	};

	struct Texture {
		uint32_t m_flags;
		bgfx::UniformHandle m_sampler;
		bgfx::TextureHandle m_texture;
		uint8_t m_stage;
	};

	uint64_t m_state;
	uint8_t m_numTextures;
	bgfx::ProgramHandle m_program;
	uint8_t m_viewId;
	Texture m_textures[4];
};

