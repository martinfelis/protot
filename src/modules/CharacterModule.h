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

struct CharacterEntity {
	/// Render entity
	Entity *mEntity = nullptr;
	Vector3f mPosition;
	Vector3f mVelocity;
	CharacterController mController;

	RigidBodyDynamics::Model* mRigModel = nullptr;

	CharacterEntity ();
	~CharacterEntity ();

	void Reset() {
		mPosition.setZero();
		mVelocity.setZero();
		mController.reset();
	}

	bool LoadRig (const char* filename);

	void Update(float dt); 
};

void ShowCharacterPropertiesWindow (CharacterEntity* character);
