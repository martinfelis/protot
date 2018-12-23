#pragma once

#include <cstdint>
#include <glad/glad.h>

#include <map>
#include <vector>

#include "math_types.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"

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
	float mShadowBias = 0.003;
	float mShowCascadesAlpha = 0.0f;

	bool mDebugDrawSplitViewBounds = false;
	bool mDebugDrawSplitWorldBounds = false;
	bool mDebugDrawSplitLightBounds = false;

	Matrix44f mLightSpaceMatrix;

	Vector4f mShadowSplits = Vector4f (0.0, 0.05, 0.3, 1.0);

	Matrix44f mSplitViewFrustum[4];
	Matrix44f mSplitLightFrustum[4];
	Vector4f mSplitBias = Vector4f (0.001f, 0.001f, 0.001f, 0.001f);
	RenderTarget mSplitTarget[4];
	BBox mSplitBoundsLightSpace[4];
	BBox mSplitBoundsWorldSpace[4];
	Camera mSplitCamera[4];

	Light() :
		mPosition (Vector3f(0.f, 3, 0.0f)),
		mDirection (Vector3f(1.f, 1.f, 1.f)),
		mShadowMapBias (0.004f),
		mShadowMapSize (2048),
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
	bool mDrawDebugCamera = false;

	int mWidth = 1;
	int mHeight = 1;
	int mSceneAreaWidth = 1;
	int mSceneAreaHeight = 1;

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

	RenderTarget mForwardRenderingTarget;
	RenderTarget mDeferredLightingTarget;
	RenderTarget mSSAOTarget;
	RenderTarget mSSAOBlurTarget;
	RenderTarget mRenderOutput;

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
	void ResizeTargets(int width, int height);
	void RenderGl();
	void RenderScene(RenderProgram &program, const Camera& camera);
	void DebugDrawShadowCascades();
	void DrawGui();

	void InitializeSSAOKernelAndNoise();
};
