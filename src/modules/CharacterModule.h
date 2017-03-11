#pragma once

#include <cstdint>

#include <vector>
#include <unordered_map>

#include "math_types.h"

#include "Globals.h"

#include "imgui_protot_ext.h"
#include "rbdl/rbdl.h"
#include "rbdl/addons/luamodel/luamodel.h"

namespace RigidBodyDynamics {
	struct Model;
}

struct CharacterController {
	enum ControllerState {
		ControlStateJump,
		ControlStateLast
	};

	bool mState[ControlStateLast];

	Vector3f mDirection = Vector3f::Zero();

	void reset() {
		for (int i = 0; i < ControlStateLast; i++) {
			mState[i] = false;
		}

		mDirection.setZero();
	}

	CharacterController() {
		reset();
	};
};

struct Animation {
	bool Load(const char* filename);
	void Sample (float cur_time, VectorNf& state);
	bool mIsLooped = true;
	float mDuration;
	std::vector<VectorNf> mFrames;
	std::vector<float> mFrameTimes;
};

struct CharacterEntity {
	/// Render entity
	Entity *mEntity = nullptr;
	Vector3f mPosition;
	Vector3f mVelocity;
	CharacterController mController;

	RigidBodyDynamics::Model* mRigModel = nullptr;
	struct RigState {
		VectorNf q;
	};
	std::vector<int> mBoneFrameIndices;
	RigState mRigState;

	Animation mAnimation;
	float mAnimTime;

	CharacterEntity ();
	~CharacterEntity ();

	void Reset() {
		mPosition.setZero();
		mVelocity.setZero();
		mController.reset();
	}

	bool LoadRig (const char* filename);
	void UpdateBoneMatrices();

	void Update(float dt); 
};

void ShowCharacterPropertiesWindow (CharacterEntity* character);
