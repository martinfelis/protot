#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#pragma once

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.

#include "FileModificationObserver.h"

#include "tinygltf/tiny_gltf.h"

#include <vector>

// Forward declarations
struct VertexArray;
struct VertexArrayMesh;

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

struct Camera {
	Vector3f mEye;
	Vector3f mPoi;
	Vector3f mUp;

	float mNear;
	float mFar;
	float mFov;
	bool mIsOrthographic;
	float mWidth;
	float mHeight;

	Matrix44f mProjectionMatrix;
	Matrix44f mViewMatrix;

	Camera() :
		mEye {5.f, 4.f, 5.f},
		mPoi {0.f, 2.f, 0.f},
		mUp  {0.f, 1.f, 0.f},
		mNear (0.1f),
		mFar (150.f),
		mFov (60.f),
		mIsOrthographic (false),
		mWidth (100.0f),
		mHeight (100.0f),

		mProjectionMatrix (
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f),
		mViewMatrix (
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f)
		{}

	void UpdateMatrices();
	void DrawGui();
};



struct RenderProgram : AFileModificationListener {
	std::string mVertexShaderFilename;
	std::string mFragmentShaderFilename;

	GLuint mProgramId = -1;

	RenderProgram() 
	{}

	RenderProgram(const std::string& vs, const std::string& fs) :
		mVertexShaderFilename(vs),
		mFragmentShaderFilename(fs)
	{}

	GLuint GetUniformLocation(const std::string& name);

	~RenderProgram();

	bool Load();

	GLuint CompileVertexShader();
	GLuint CompileFragmentShader();
	GLuint LinkProgram(GLuint vertex_shader, GLuint fragment_shader);

	GLint SetInt(const char* name, const GLint& val) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniform1i(location, val);
		return location;
	}
	GLint SetFloat(const char* name, const float& val) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniform1f(location, val);
		return location;
	}
	GLint SetVec3(const char* name, const Vector3f& vec) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniform3fv(location, 1, vec.data()); 
		return location;
	}
	GLint SetVec4(const char* name, const Vector3f& vec, float w = 1.0f)	{
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniform4f(location, vec[0], vec[1], vec[2], w); 
		return location;
	}
	GLint SetVec4(const char* name, const Vector4f& vec) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniform4fv(location, 1, vec.data()); 
		return location;
	}
	GLint SetMat44 (const char* name, const Matrix44f& mat) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniformMatrix4fv(location, 1, GL_FALSE, mat.data()); 
		return location;
	}
	GLint SetMat33 (const char* name, const Matrix33f& mat) {
		GLint location = glGetUniformLocation(mProgramId, name);
		glUniformMatrix3fv(location, 1, GL_FALSE, mat.data()); 
		return location;
	}

	void RegisterFileModification();
	virtual bool OnFileChanged(const std::string& filename);
};

struct RenderSettings;
struct RenderProgram;

struct RenderTarget {
	int mWidth = 0;
	int mHeight = 0;
	GLuint mFrameBufferId = -1;
	GLuint mColorTexture = -1;
	GLuint mDepthBuffer = -1;
	GLuint mDepthTexture = -1;
	GLuint mLinearizedDepthTexture = -1;

	typedef enum {
		EnableColor = 1,
		EnableDepth = 2,
		EnableDepthTexture = 4,
		EnableLinearizedDepthTexture = 8
	} Flags;

	int mFlags = 0;

	RenderProgram mLinearizeDepthProgram;
	VertexArray* mVertexArray = nullptr;
	VertexArrayMesh* mQuadMesh = nullptr;	

	RenderTarget() {};
	~RenderTarget();

	void Initialize(int width, int height, int flags);
	void Bind();
	void Cleanup();
	void Resize(int width, int height);

	void RenderToLinearizedDepth(const float& near, const float& far, bool is_orthographic);
};

struct Texture {
	GLuint mTextureId;
	GLuint mWidth;
	GLuint mHeight;

	~Texture();
	void MakeGrid(const int& size, const Vector3f &c1, const Vector3f &c2);
	bool Load(const char* path, int num_components = 3);
};

/**
 * Storage for (multiple) VertexArrayMeshes
 * Storage order is:
 *   (VVVV)(NNNN)(UV)(CCCC)
 */
struct VertexArray {
	GLuint mVertexBuffer = -1;
	GLuint mVertexArrayId = -1;
	GLuint mNumVertices = -1;
	GLuint mNumUsedVertices = -1;

	struct VertexData {
		union {
			struct {
				float x;
				float y;
				float z;
				float w;
				float nx;
				float ny;
				float nz;
				float s;
				float t;
				GLubyte r;
				GLubyte g;
				GLubyte b;
				GLubyte a;
			};
			struct {
				float mCoords[4];
				float mNormals[3];
				float mTexCoords[2];
				GLubyte mColor[4];
			};
		};

		VertexData() :
			x(0.0f),
			y(0.0f),
			z(0.0f),
			w(0.0f),
			nx(0.0f),
			ny(0.0f),
			nz(0.0f),
			s(0.0f),
			t(0.0f),
			r(255),
			g(255),
			b(255),
			a(255) {}

		VertexData(
				float x,
				float y,
				float z,
				float w,
				float nx,
				float ny,
				float nz,
				float s,
				float t,
				GLubyte r,
				GLubyte g,
				GLubyte b,
				GLubyte a
				) :
			x(x), y(y), z(z), w(w),
			nx(nx), ny(ny), nz(nz),
			s(s), t(t),
			r(r), g(g), b(b), a(a) {}

		VertexData& operator= (const VertexData& data) {
			x = data.x;
			y = data.y;
			z = data.z;
			w = data.w;
			nx = data.nx;
			ny = data.ny;
			nz = data.nz;
			r = data.r;
			g = data.g;
			b = data.b;
			a = data.a;
			
			return *this;
		}
	};

	~VertexArray();

	void Initialize(const int& size, GLenum usage);
	void Cleanup();
	GLuint AllocateMesh(const int& size);
	void Bind();
	bool IsBound();
};

struct VertexArrayMesh {
	VertexArray* mVertexArray = (VertexArray*) -1;
	void* mOffsetPtr = (void*) -1;
	GLuint mVertexCount = -1;

	GLuint mIndexOffset = -1;
	GLuint mIndexBuffer = -1;
	GLuint mIndexCount = -1;

	void Initialize(VertexArray &array, const int& size);

	void SetData(
			const VertexArray::VertexData* data,
			const int& count
			);

	void SetData(
			const std::vector<Vector4f> &coords,
			const std::vector<Vector3f> &normals,
			const std::vector<Vector2f> &uvs,
			const std::vector<Vector4f> &colors,
			const std::vector<GLuint> &indices
			);

	void SetIndexData(const GLuint* indices, const int& count);

	void Draw(GLenum mode);
};

struct AssetFile {
	std::string mFilename;
	tinygltf::Model mGLTFModel;
	std::vector<GLuint> mBuffers;

	bool Load(const char* filename);
	void LoadBuffers();
	void DrawMesh(const tinygltf::Mesh &mesh, const Matrix44f &matrix);
	void DrawNode(const tinygltf::Node &node, const Matrix44f &matrix);
	void DrawModel();
	void DrawNodeGui(const tinygltf::Node& node);
	void DrawGui();
};

struct RenderCommand {
	typedef enum {
		EnableShadowPass = 1
	} Flags;

	int mFlags;
	GLenum mDrawMode;
	Vector4f mColor = Vector4f (1.0f, 1.0f, 1.0f, 1.0f);

	Matrix44f mTransform;
	VertexArrayMesh *mMesh;
};

#endif
