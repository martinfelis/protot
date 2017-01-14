#pragma once

#include <cstdint>

#include <vector>
#include <unordered_map>

#include "math_types.h"

#include "Globals.h"

#include "imgui_protot_ext.h"

struct CharacterController {
	enum ControllerState {
		ControlStateJump,
		ControlStateLast
	};

	bool state[ControlStateLast];

	Vector3f direction = Vector3f::Zero();

	void reset() {
		for (int i = 0; i < ControlStateLast; i++) {
			state[i] = false;
		}

		direction.setZero();
	}

	CharacterController() {
		reset();
	};
};

struct CharacterEntity {
	/// Render entity
	Entity *entity = nullptr;
	Vector3f position;
	Vector3f velocity;
	CharacterController controller;

	CharacterEntity ();
	~CharacterEntity ();

	void reset() {
		position.setZero();
		velocity.setZero();
		controller.reset();
	}

	void update(float dt); 
};

void ShowCharacterPropertiesWindow (CharacterEntity* character);
