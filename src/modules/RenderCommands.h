#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DebugCube = 1,
	DebugFrame = 2,
	DebugSphere = 3,
	DebugBone = 4
} RenderObject;

void RenderCommandsClear ();

void RenderSubmit (
		RenderObject object,
		float* transform,
		float color[4]
		);

#ifdef __cplusplus
}
#endif
