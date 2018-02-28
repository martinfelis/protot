#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#pragma once

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.

#include <vector>

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

struct RenderProgram {
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
};

struct RenderSettings;


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

	RenderTarget() {};
	RenderTarget(int width, int height, int flags);
	~RenderTarget();

	void Cleanup();
	void Resize(int width, int height);
	void RenderToLinearizedDepth(bool render_to_depth);
};

struct Texture {
	GLuint mTextureId;
	GLuint mWidth;
	GLuint mHeight;

	~Texture();
	void MakeGrid(const int& size, const Vector3f &c1, const Vector3f &c2);
	bool Load(const char* path, int num_components = 3);
};

struct Mesh {
	GLuint mVertexArrayId = -1;
	GLuint mVertexBuffer = -1;
};

/**
 * Multiple VertexArrayMeshes can be stored in a single VertexArray.
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
				float u;
				float v;
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

	void Draw(GLenum mode);
};

#endif
