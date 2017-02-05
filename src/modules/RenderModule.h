#pragma once

#include <cstdint>

#include <map>
#include <vector>

#include "math_types.h"

#include <bgfx/bgfx.h>

#include "RenderUtils.h"

struct Entity;

struct Camera {
	Vector3f eye;
	Vector3f poi;
	Vector3f up;

	float near;
	float far;
	float fov;
	bool orthographic;
	float width;
	float height;

	float mtxProj[16];
	float mtxView[16];
	float mtxEnv[16];

	Camera() :
		eye {5.f, 4.f, 5.f},
		poi {0.f, 2.f, 0.f},
		up  {0.f, 1.f, 0.f},
		near (0.1f),
		far (150.f),
		fov (60.f),
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
			0.f, 0.f, 0.f, 1.f},
		mtxEnv {
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
	float pos[4];
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
		pos {0.f, 10.f, 10.f, 1.0f},
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
		shadowMapBias (0.004f),
		shadowMapSize (2048),
		near (0.1f),
		far (100.f),
		area (10.f),
		enabled (false)
	{
	}
};

struct Transform {
	Quaternion rotation = Quaternion (0.0f, 0.0f, 0.0f, 1.0f);
	Vector3f translation = Vector3f (0.0f, 0.0f, 0.0f);
	Vector3f scale = Vector3f (1.0f, 1.0f, 1.0f);

	Transform () {};

	Transform (
			const Vector3f &translation,
			const Quaternion &rotation,
			const Vector3f &scale)
		:
		translation(translation),
		rotation(rotation),
		scale(scale)
	{}

	Transform (const Matrix44f& mat) {
		fromMatrix(mat);
	}
	
	Matrix44f toMatrix() const {
		Matrix44f result;

		Matrix33f scale_mat (
				scale[0], 0.0f, 0.0f,
				0.0f, scale[1], 0.0f,
				0.0f, 0.0f, scale[2]
				);
		result.block<3,3>(0,0) = scale_mat * rotation.toMatrix();
		result.block<1,3>(3,0) = translation.transpose();
		result.block<3,1>(0,3) = Vector3f::Zero();
		result(3,3) = 1.0f;

		return result;
	}

	void fromMatrix(const Matrix44f &matrix) {
		// Extract rotation matrix and the quaternion
		Matrix33f rot_matrix (matrix.block<3,3>(0,0));

		Vector3f row0 = rot_matrix.block<1,3>(0,0).transpose();
		Vector3f row1 = rot_matrix.block<1,3>(1,0).transpose();
		Vector3f row2 = rot_matrix.block<1,3>(2,0).transpose();

		scale.set(
				row0.norm(),
				row1.norm(),
				row2.norm()
				);

		rot_matrix.block<1,3>(0,0) = (row0 / scale[0]).transpose();
		rot_matrix.block<1,3>(1,0) = (row1 / scale[1]).transpose();
		rot_matrix.block<1,3>(2,0) = (row2 / scale[2]).transpose();

		rotation = Quaternion::fromMatrix(rot_matrix).normalize();

		row0 = rot_matrix.block<1,3>(0,0).transpose();
		row1 = rot_matrix.block<1,3>(1,0).transpose();
		row2 = rot_matrix.block<1,3>(2,0).transpose();

		Vector3f trans (
				matrix(3,0), 
				matrix(3,1), 
				matrix(3,2)
				);

		translation = trans;
	}
	Transform operator*(const Transform &other) const {
		Matrix44f this_mat (toMatrix());
		Matrix44f other_mat (other.toMatrix());

		return Transform(this_mat * other_mat);
	}
	Vector3f operator*(const Vector3f &vec) const {
		assert(false);
		return Vector3f::Zero();
	}

	static Transform fromTrans(
			const Vector3f &translation
			) {
		return Transform (
				translation, 
				Quaternion(0.0f, 0.0f, 0.0f, 1.0f), 
				Vector3f(1.0f, 1.0f, 1.0f)
				);
	}

	static Transform fromTransRot(
			const Vector3f &translation,
			const Quaternion &rotation
			) {
		return Transform (
				translation, 
				rotation, 
				Vector3f(1.0f, 1.0f, 1.0f)
				);
	}

	static Transform fromTransRotScale(
			const Vector3f &translation,
			const Quaternion &rotation,
			const Vector3f &scale
			) {
		return Transform (translation, rotation, scale);
	}
};

struct MeshHierarchy {
	~MeshHierarchy()
	{
		for (bgfxutils::Mesh* mesh : meshes) {
			meshUnload(mesh);
		}
	}
	std::vector<bgfxutils::Mesh*> meshes;

	/// index of the parent. Children must have higher indices than heir
	//  parents
	std::vector<int> parent;
	/// Transforms relative to their parents.
	std::vector<Transform> localTransforms;
	/// Absolute transforms.
	std::vector<Matrix44f> meshMatrices;

	void addMesh(
			const int parent_idx, 
			const Transform& transform, 
			bgfxutils::Mesh* mesh) {
		assert (parent_idx == -1 || parent_idx < parent.size());
		parent.push_back(parent_idx);
		localTransforms.push_back(transform);
		meshes.push_back(mesh);

		if (parent_idx != -1) {
			meshMatrices.push_back(transform.toMatrix() * meshMatrices[parent_idx]);
		} else {
			meshMatrices.push_back(transform.toMatrix());
		}

	}
	void updateMatrices(const Matrix44f &world_transform);
	void submit(const RenderState *state);
};

struct Entity {
	Transform transform;
	float color[4];
	MeshHierarchy mesh;

	Entity() :
		color { 1.f, 1.f, 1.f, 1.f }
	{}
};

struct LightProbe
{
	enum Enum
	{
		Bolonga,
		Kyoto,

		Count
	};

	void load(const char* _name);

	void destroy()
	{
		bgfx::destroyTexture(m_tex);
		bgfx::destroyTexture(m_texIrr);
	}

	bgfx::TextureHandle m_tex;
	bgfx::TextureHandle m_texIrr;
};

struct Path {
	std::vector<Vector3f> points;
	std::vector<Vector4f> colors;
	float thickness = 0.1f;
	float miter = 0.0f;
	bgfx::DynamicVertexBufferHandle mVertexBufferHandle = BGFX_INVALID_HANDLE;
	bgfx::DynamicIndexBufferHandle mIndexBufferHandle = BGFX_INVALID_HANDLE;

	~Path();
	void UpdateBuffers();
};

struct DebugCommand {
	enum CommandType {
		Line,
		Axes,
		Circle,
		Invalid
	};

	CommandType type;

	Vector3f from;
	float radius;
	Vector3f to;
	Vector4f color = Vector4f(1.f, 1.f, 1.f, 1.f);
};

struct Renderer {
	bool initialized;
	bool drawDebug;
	bool drawFloor = true;
	bool drawSkybox = true;
	uint32_t width;
	uint32_t height;

	bgfx::UniformHandle sceneDefaultTextureSampler;
	bgfx::TextureHandle sceneDefaultTexture;

	std::vector<bgfxutils::Mesh*> meshes;
	typedef std::map<std::string, unsigned int> MeshIdMap;
	MeshIdMap meshIdMap;

	LightProbe mLightProbes[LightProbe::Count];
	LightProbe::Enum mCurrentLightProbe;

	std::vector<Entity*> entities;

	std::vector<Camera> cameras;
	std::vector<Light> lights;
	std::vector<Path> debugPaths;
	std::vector<DebugCommand> debugCommands;

	uint16_t activeCameraIndex;

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

	// check whether shader files were modified and reload them. Returns
	// true on success, otherwise false
	bool updateShaders();

	Entity* createEntity();
	bool destroyEntity (Entity* entity);

	bgfxutils::Mesh* loadMesh(const char* filename);

	// debug commands
	void drawDebugLine (
			const Vector3f &from,
			const Vector3f &to,
			const Vector3f &color);

	void drawDebugAxes (
			const Vector3f &pos,
			const Matrix33f &orientation,
			const float &scale);

	void drawDebugCircle (
			const Vector3f &pos,
			const Vector3f &normal,
			const float radius,
			const Vector4f &color = Vector4f (1.f, 1.f, 1.f, 1.f));

	void drawDebugSphere (
			const Vector3f &pos,
			const float radius,
			const Vector4f &color = Vector4f(1.f, 1.f, 1.f, 1.f));
};

struct RenderProgram {
	bgfx::ProgramHandle program;
	std::string vertexShaderFileName;
	int vertexShaderFileModTime;
	std::string fragmentShaderFileName;
	int fragmentShaderFileModTime;

	RenderProgram () :
		vertexShaderFileName(""),
		vertexShaderFileModTime(-1),
		fragmentShaderFileName(""),
		fragmentShaderFileModTime(-1)
	{
		program = BGFX_INVALID_HANDLE;
	}

	RenderProgram (
			const char* vertex_shader_file_name,
			const char* fragment_shader_file_name
			)
		: 
			vertexShaderFileName(vertex_shader_file_name),
			vertexShaderFileModTime(-1),
			fragmentShaderFileName(fragment_shader_file_name),
			fragmentShaderFileModTime(-1)
	{
		program = BGFX_INVALID_HANDLE;
	}

	bool reload();
	bool checkModified() const;
	bool valid() const {
		return bgfx::isValid(program);
	}
};

struct RenderState {
	enum  {
		Skybox,
		ShadowMap,
		Scene,
		SceneTextured,
		Lines,
		LinesOccluded,
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
	RenderProgram m_program;
	uint8_t m_viewId;
	Texture m_textures[4];
};

