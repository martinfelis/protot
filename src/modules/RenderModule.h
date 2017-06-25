#pragma once

#include <cstdint>

#include <map>
#include <vector>

#include "math_types.h"

#include <bgfx/bgfx.h>

#include "Globals.h"
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
	Vector3f pos;
	Vector3f dir;

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
		pos (Vector3f(0.f, 10.f, 10.f)),
		dir (Vector3f(-1.f, -1.f, -1.f)),
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

	static Transform fromTransRot(
			const Vector3f &translation,
			const Matrix33f &rotation
			) {
		return Transform (
				translation, 
				Quaternion::fromMatrix(rotation), 
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

struct Skeleton {
	/// index of the mParent. Children must have higher indices than heir
	//  mParents
	std::vector<int> mParent;
	/// Transforms relative to their mParents.
	std::vector<Transform> mLocalTransforms;
	/// Absolute transforms.
	std::vector<Matrix44f> mBoneMatrices;

	int AddBone(
			const int parent_index, 
			const Transform& transform
			) {
		assert (parent_index == -1 || parent_index < mParent.size());
		mParent.push_back(parent_index);
		mLocalTransforms.push_back(transform);

		if (parent_index != -1) {
			mBoneMatrices.push_back(transform.toMatrix() * mBoneMatrices[parent_index]);
		} else {
			mBoneMatrices.push_back(transform.toMatrix());
		}

		return mBoneMatrices.size() - 1;
	}
	void UpdateMatrices(const Matrix44f &world_transform);
	int Length() const {
		return mBoneMatrices.size();
	}
};

struct SkeletonMeshes {
	Skeleton& mSkeleton;
	typedef std::pair<Mesh*, int> MeshBoneIndex;
	std::vector<MeshBoneIndex> mMeshBoneIndices;

	SkeletonMeshes(Skeleton &skeleton) :
		mSkeleton(skeleton)
	{}

	~SkeletonMeshes() {
		for(MeshBoneIndex& mesh_bone : mMeshBoneIndices) {
			delete mesh_bone.first;
		}
	}

	void AddMesh (Mesh* mesh, int bone_index) {
		mMeshBoneIndices.push_back (MeshBoneIndex (mesh, bone_index));
	}

	const Matrix44f GetBoneMatrix(int index) const {
		assert (index >= 0 && index < Length());
		assert (mMeshBoneIndices[index].second < mSkeleton.mBoneMatrices.size());

		return mSkeleton.mBoneMatrices[mMeshBoneIndices[index].second];
	}

	const Mesh* GetMesh(int index) const {
		assert (index >= 0 && index < Length());
		return mMeshBoneIndices[index].first;
	}

	int Length() const {
		return mMeshBoneIndices.size();
	}
};

struct Entity {
	Transform mTransform;
	Vector4f mColor;
	Skeleton mSkeleton;
	SkeletonMeshes mSkeletonMeshes;
	bool mDrawEntity = false;

	Entity() :
		mColor (1.f, 1.f, 1.f, 1.f ),
		mSkeletonMeshes(mSkeleton)
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
	uint32_t view_offset_x = 0;
	uint32_t view_offset_y = 0;
	uint32_t view_width = 1;
	uint32_t view_height = 1;

	bgfx::UniformHandle sceneDefaultTextureSampler;
	bgfx::TextureHandle sceneDefaultTexture;

	LightProbe mLightProbes[LightProbe::Count];
	LightProbe::Enum mCurrentLightProbe;

	std::vector<Entity*> entities;

	std::vector<Camera> cameras;
	std::vector<Light> lights;
	std::vector<Path> debugPaths;
	std::vector<DebugCommand> debugCommands;

	Mesh debugBoneMesh;

	uint16_t activeCameraIndex;

	Renderer() :
		initialized(false),
		drawDebug(false),
		view_width (0),
		view_height (0)
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
	void resize (int x, int y, int width, int height);

	// check whether shader files were modified and reload them. Returns
	// true on success, otherwise false
	bool updateShaders();

	Entity* createEntity();
	bool destroyEntity (Entity* entity);

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

