#include "Globals.h"
#include "RenderCommands.h"
#include "RenderModule.h"

#include <vector>
#include <cstring>

using namespace std;

void RenderCommandsClear() {
	if (gRenderer == NULL) {
		gLog ("Warning: Cannot clear render commands: no renderer found!");
		return;
	}
	gRenderer->mRenderCommands.clear();
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


