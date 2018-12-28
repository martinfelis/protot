#include "RenderCommands.h"

#include <vector>
#include <cstring>

struct RenderCommand {
	RenderObject mObject;
	float mTransform[16];
	float mColor[4];
};
