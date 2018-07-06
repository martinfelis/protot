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

struct ShadowSplitInfo {
	BBox mBoundsLight;
	BBox mBoundsWorld;
	Matrix44f mViewFrustum;
	Matrix44f mFrustum;
	RenderTarget mShadowMapTarget; 
	Camera mCamera;
};

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
	float mShadowBias = 0.003;

	bool mDebugDrawSplitViewBounds = true;
	bool mDebugDrawSplitWorldBounds = true;
	bool mDebugDrawSplitLightBounds = true;

	Matrix44f mLightSpaceMatrix;

	Vector4f mShadowSplits = Vector4f (0.0, 0.1, 0.4, 1.0);
	ShadowSplitInfo mSplits[4];

	Light() :
		mPosition (Vector3f(0.f, 3, 0.0f)),
		mDirection (Vector3f(1.f, 1.f, 1.f)),
		mShadowMapBias (0.004f),
		mShadowMapSize (512),
		mNear (-10.0f),
		mFar (15.f),
		mBBoxSize (35.f),
		mLightSpaceMatrix(Matrix44f::Identity())
	{
	}

	void Initialize();
	void UpdateMatrices();
	void DrawGui();
	void UpdateSplits(const Camera& camera);
};

struct RendererSettings;

struct Renderer {
	RendererSettings* mSettings = nullptr;

	bool mInitialized = false;
	bool mIsSSAOEnabled = false;
	bool mUseDeferred = false;
	bool mDrawDebugCamera = true;

	uint32_t mWidth = 1;
	uint32_t mHeight = 1;
	uint32_t mSceneAreaWidth = 1;
	uint32_t mSceneAreaHeight = 1;

	Light mLight;
	Camera mCamera;
	Camera mDebugCamera;

	Texture mDefaultTexture;

	RenderProgram mSimpleProgram;
	RenderProgram mDeferredGeometry;
	RenderProgram mDeferredLighting;
	RenderProgram mDefaultProgram;
	RenderProgram mRenderQuadProgramColor;
	RenderProgram mRenderQuadProgramDepth;
	RenderProgram mSSAOProgram;
	RenderProgram mBlurSSAOProgram;

	RenderTarget mRenderTarget;
	RenderTarget mDeferredLightingTarget;
	RenderTarget mSSAOTarget;
	RenderTarget mSSAOBlurTarget;
	RenderTarget mPostprocessTarget;

	GLTextureRef mRenderTextureRef = { (int)0xbadface };
	GLTextureRef mPositionTextureRef = { (int)0xbadface };
	GLTextureRef mNormalTextureRef = { (int)0xbadface };

	float mSSAORadius = 0.5f;
	float mSSAOBias = 0.025f;
	float mSSAOPower = 1.0f;
	int mSSAOKernelSize = 64;
	std::vector<Vector3f> mSSAOKernel;
	GLuint mSSAONoiseTexture = -1;
	int mSSAOBlurSize = 1;
	bool mSSAODisableColor = false;

	Renderer() :
		mInitialized(false),
		mWidth (0),
		mHeight (0)
	{ }

	void Initialize(int width, int height);
	void Shutdown();
	void CheckRenderBuffers();
	void RenderGl();
	void RenderScene(RenderProgram &program, const Camera& camera);
	void DebugDrawShadowCascades();
	void DrawGui();

	void InitializeSSAOKernelAndNoise();
};
