#pragma once

struct Timer {
	float mCurrentTime = 0.0f;
	float mFrameTime = 0.0f;
	float mDeltaTime = 0.0f;
	bool mPaused = false;
};
