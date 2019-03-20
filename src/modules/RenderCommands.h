#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DebugCube = 1,
	DebugFrame = 2,
	DebugSphere = 3,
	DebugBone = 4,
	StaticMesh = 5,
	Line = 6,
	SkinnedMesh = 7
} RenderObject;

typedef enum {
	PassDepth     = 1,
	PassShadowMap = 2,
	PassSSAO      = 3,
	PassLighting  = 4,
	PassCount
} RenderPass;

typedef enum {
	PrimitivePoints
	PrimitiveLines,
	PrimitiveTriangles,
} RenderPrimitive;

typedef struct {
	float mColor[4];
	float mTransform[16];
	GLuint mVAId;
	GLuint mVBId;
	GLuint mAlbedoTexture;
	RenderPrimitive mPrimitive;
} RenderCommand;

void RenderCommandsClear ();

RenderCommand *GetRenderCommand(RenderPass pass);

void RenderSubmit (
		RenderPass pass,
		RenderObject object,
		float* transform,
		float color[4]
		);

#ifdef __cplusplus
}
#endif
