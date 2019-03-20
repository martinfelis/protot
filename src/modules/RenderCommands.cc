#include "Globals.h"
#include "RenderCommands.h"
#include "RenderModule.h"

#include <vector>
#include <cstring>

using namespace std;

const int cNumMaxRenderCommands = 1024;

RenderCommand sRenderCommands[PassCount][cNumMaxRenderCommands];
int sRenderCommandCount[PassCount];

void RenderCommandReset(RenderCommand *command) {
	assert (command != nullptr);

	command->mColor[0] = 0.f;
	command->mColor[1] = 0.f;
	command->mColor[2] = 0.f;
	command->mColor[3] = 0.f;

	for (int i = 0; i < 16; i++) {
		command->mTransform[i] = 0.f;
	}
	for (int i = 0; i < 4; i++) {
		command->mTransform[i * 4 + i] = 1.f;
	}

	command->mVAId = -1;
	command->mVBId = -1;

	command->mAlbedoTexture = -1;
	command->mPrimitive = PrimitiveTriangles;
};

void RenderCommandsClear() {
	for (int i = 0; i < PassCount; i++) {
		for (int j = 0; j < sRenderCommandCount[i]; j++) {
			RenderCommandReset(&sRenderCommands[i][j]);
		}
		sRenderCommandCount[i] = 0;
	}

	if (gRenderer == NULL) {
		gLog ("Warning: Cannot clear render commands: no renderer found!");
		return;
	}
	gRenderer->mRenderCommands.clear();
}

RenderCommand *GetRenderCommand(RenderPass pass) {
	int& pass_count = sRenderCommandCount[pass];
	if (pass_count == cNumMaxRenderCommands) {
		gLog ("Error: Reached max number of render commands: %d for pass %d",
				cNumMaxRenderCommands, pass);
		return nullptr;
	}

	pass_count++;

	return &sRenderCommands[pass][pass_count];
}

void RenderSubmit (
		RenderObject object,
		float* transform,
		float color[4]
		) {
	RenderCommand command;
	command.mObject = object;
	memcpy (command.mTransform, transform, sizeof (float) * 16);
	memcpy (command.mColor, color, sizeof(float) * 4);

	gRenderer->mRenderCommands.push_back(command);
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


