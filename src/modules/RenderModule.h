#pragma once

#include <cstdint>

#include <map>
#include <vector>

#include "math_types.h"

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.

#include "Globals.h"
#include "RenderUtils.h"

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
		mWidth (-1.f),
		mHeight (-1.f),

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

struct Light {
	Vector3f pos;
	Vector3f dir;

	float mViewMatrix[16];
	float mProjectionMatrix[16];
	float mtxLight[16];
	float mtxShadow[16];

	float shadowMapBias;
	uint16_t shadowMapSize;

	bool enabled;
	float near;
	float far;
	float area;

	Light() :
		pos (Vector3f(0.f, 10.f, 10.f)),
		dir (Vector3f(-1.f, -1.f, -1.f)),
		mViewMatrix {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		},
		mProjectionMatrix {
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

struct RendererSettings;

struct Renderer {
	RendererSettings* mSettings = nullptr;

	bool mInitialized = false;
	uint32_t mWidth = 1;
	uint32_t mHeight = 1;

	Camera mCamera;
	Mesh mMesh;
	Mesh mPlane;
	Mesh mCoordinateSystem;

	Texture mDefaultTexture;

	RenderProgram mDefaultProgram;
	GLuint muDefaultModelViewProjection;
	GLuint muDefaultColor;

	RenderTarget mRenderTarget;

	GLuint mRenderQuadVertexArrayId;
	GLuint mRenderQuadVertexBufferId;

	RenderProgram mRenderQuadProgramColor;
	GLuint muRenderQuadModelViewProj;
	GLuint muRenderQuadTexture;
	GLuint muRenderQuadTime;

	RenderProgram mRenderQuadProgramDepth;
	GLuint muRenderQuadDepthModelViewProj;
	GLuint muRenderQuadDepthNear;
	GLuint muRenderQuadDepthFar;

	Renderer() :
		mInitialized(false),
		mWidth (0),
		mHeight (0)
	{ }

	void Initialize(int width, int height);
	void Shutdown();
	void RenderGl();
	void RenderGui();
	void Resize (int width, int height);
};
