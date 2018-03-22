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

	Camera mCamera;

	float mShadowMapBias;
	uint16_t mShadowMapSize;

	float mNear;
	float mFar;
	float mBBoxSize;

	Matrix44f mLightSpaceMatrix;

	Light() :
		mPosition (Vector3f(0.f, 3, 0.0f)),
		mDirection (Vector3f(1.f, 1.f, 1.f)),
		mShadowMapBias (0.004f),
		mShadowMapSize (1024),
		mNear (-10.0f),
		mFar (15.f),
		mBBoxSize (35.f),
		mLightSpaceMatrix(Matrix44f::Identity())
	{
	}

	void Initialize();
	void UpdateMatrices();
	void DrawGui();
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
	RenderProgram mRenderQuadProgramColor;
	RenderProgram mRenderQuadProgramDepth;

	RenderTarget mRenderTarget;
	GLTextureRef mRenderTextureRef = { (int)0xbadface };

	Renderer() :
		mInitialized(false),
		mWidth (0),
		mHeight (0)
	{ }

	void Initialize(int width, int height);
	void Shutdown();
	void RenderGl();
	void RenderScene(RenderProgram &program, const Camera& camera);
	void DrawGui();
};
