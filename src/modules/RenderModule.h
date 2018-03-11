#pragma once

#include <cstdint>

#include <map>
#include <vector>

#include "math_types.h"

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include "Globals.h"
#include "RenderUtils.h"

struct Light {
	Vector3f mPosition;
	Vector3f mDirection;

	RenderTarget mShadowMapTarget;
	RenderProgram mShadowMapProgram;

	float mShadowMapBias;
	uint16_t mShadowMapSize;

	float mNear;
	float mFar;
	float mBBoxSize;

	Matrix44f mLightProjection;
	Matrix44f mLightView;
	Matrix44f mLightSpaceMatrix;

	Light() :
		mPosition (Vector3f(0.f, 10.f, 10.f)),
		mDirection (Vector3f(-1.f, -1.f, -1.f)),
		mShadowMapBias (0.004f),
		mShadowMapSize (1024),
		mNear (0.1f),
		mFar (100.f),
		mBBoxSize (10.f),
		mLightProjection(Matrix44f::Identity()),
		mLightView(Matrix44f::Identity()),
		mLightSpaceMatrix(Matrix44f::Identity())
	{
	}

	void Initialize();
	void UpdateMatrices();
};

struct RendererSettings;

struct Renderer {
	RendererSettings* mSettings = nullptr;

	bool mInitialized = false;
	uint32_t mWidth = 1;
	uint32_t mHeight = 1;
	uint32_t mSceneAreaWidth = 1;
	uint32_t mSceneAreaHeight = 1;

	Light mLight;
	Camera mCamera;

	Texture mDefaultTexture;

	RenderProgram mSimpleProgram;

	RenderProgram mDefaultProgram;
	GLuint muDefaultModelViewProjection;
	GLuint muDefaultColor;

	RenderTarget mRenderTarget;
	GLTextureRef mRenderTextureRef = { (int)0xbadface };

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
	void RenderScene(RenderProgram &program);
	void RenderGui();
};
