#include <map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <sstream>

#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"
#include <bx/fpumath.h>

#include "RuntimeModule.h"
#include "Globals.h"
#include "modules/RenderModule.h"
#include "Serializer.h"

#include "CharacterModule.h"

using namespace std;

const float cJumpVelocity = 4.0f;
const float cVelocityDamping = 4.0f;
const float cGravity = 9.81f;
const float cGroundAcceleration = 30.0f;
const float cCharacterHeight = 1.8f;
const float cCharacterWidth = 1.f;

CharacterEntity::CharacterEntity() {
	entity = gRenderer->createEntity();
	position = Vector3f (0.f, 0.0f, 0.0f);
	cout << "Creating render entity ... success!" << endl;

	cout << "Creating render entity mesh ..." << endl;

	// Build the snowman
	entity->mesh.addMesh(
			- 1,
			Transform::fromTrans(
				Vector3f (0.0f, 0.9 * 0.5f, 0.0f)
				),
			Mesh::sCreateUVSphere(45, 45, 0.9)
			);

	entity->mesh.addMesh(
			0,
			Transform::fromTrans(
				Vector3f (0.0f, 0.55f, 0.0f)
				),
			Mesh::sCreateUVSphere (45, 45, 0.7)
			);

	entity->mesh.addMesh(
			1,
			Transform::fromTrans(
				Vector3f (0.0f, 0.4f, 0.0f)
				),
			Mesh::sCreateUVSphere (45, 45, 0.5)
			);

	//	state->character->entity->mesh = bgfxutils::createCuboid (1.f, 1.f, 1.f);
	//	state->character->entity->mesh = bgfxutils::createCylinder (20);
	cout << "Creating render entity mesh ... success!" << endl;
}

CharacterEntity::~CharacterEntity() {
	gRenderer->destroyEntity(entity);
	entity = nullptr;
}

static float cur_time = 0.0f;

void CharacterEntity::update(float dt) {
	Vector3f controller_acceleration (
			controller.direction[0] * cGroundAcceleration,
			controller.direction[1] * cGroundAcceleration,
			controller.direction[2] * cGroundAcceleration
			);

	Vector3f gravity (0.0f, -cGravity, 0.0f);
	Vector3f damping (
			-velocity[0] * cVelocityDamping,
			0.0f,
			-velocity[2] * cVelocityDamping
			);

	Vector3f acceleration = controller_acceleration + gravity + damping;

	velocity = velocity + acceleration * dt;

	if (position[1] == 0.0f 
			&& controller.state[CharacterController::ControlStateJump]) {
		velocity[1] = cJumpVelocity;	
	}

	// integrate position
	position += velocity * dt;

	if (position[1] < 0.f) {
		position[1] = 0.f;
		velocity[1] = 0.0f;
	}

	// apply transformation
	entity->transform.translation.set(
			position[0],
			position[1],
			position[2]);

	entity->mesh.updateMatrices(entity->transform.toMatrix());

	cur_time += dt;

	gRenderer->drawDebugSphere (Vector3f (0.f, 1.3 + sin(cur_time * 2.f), 0.f), 2.2f);
}

void ShowCharacterPropertiesWindow (CharacterEntity* character) {
	assert (character != nullptr);
	ImGui::SetNextWindowSize (ImVec2(600.f, 300.0f), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos (ImVec2(400.f, 16.0f), ImGuiSetCond_Once);
	ImGui::Begin("Character");

	if (ImGui::Button ("Reset")) {
		character->reset();
	}
	
	ImGui::DragFloat3 ("Position", character->position.data(), 0.01, -10.0f, 10.0f);
	ImGui::DragFloat3 ("Velocity", character->velocity.data(), 0.01, -10.0f, 10.0f);


	for (int i = 0; i < character->entity->mesh.meshes.size(); ++i) {
		char buf[32];
		snprintf (buf, 32, "Mesh %d", i);

		ImGuiTreeNodeFlags node_flags = 0;

		bool node_open = ImGui::TreeNodeEx(
				buf,
				node_flags);

		if (node_open) {
			Transform &transform = character->entity->mesh.localTransforms[i];

			ImGui::DragFloat3 ("Position", transform.translation.data(), 0.01, -10.0f, 10.0f);
			if (ImGui::Protot::DragFloat4Normalized ("Rotation", transform.rotation.data(), 0.001, -1.0f, 1.0f)) {
				if (isnan(transform.rotation.squaredNorm())) {
					std::cout << "nan! " << transform.rotation.transpose() << std::endl;
					abort();
				}
			}
			ImGui::DragFloat3 ("Scale", transform.scale.data(), 0.01, 0.001f, 10.0f);

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

struct module_state {
};

static struct module_state *module_init() {
	std::cout << "Module init called" << std::endl;
	module_state *state = (module_state*) malloc(sizeof(*state));
	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
//	SerializeVec3(*serializer, "protot.TestModule.entity.position", state->character->position);
}

static void module_finalize(struct module_state *state) {
	std::cout << "Module finalize called" << std::endl;
	free(state);
}

static void module_reload(struct module_state *state, void* read_serializer) {
	std::cout << "Module reload called. State: " << state << std::endl;
	// load the state of the entity
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}
}

static void module_unload(struct module_state *state, void* write_serializer) {
	// serialize the state of the entity
	if (write_serializer != nullptr) {
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}
}

static bool module_step(struct module_state *state, float dt) {
	return true;
}

extern "C" {

const struct module_api MODULE_API = {
	.init = module_init,
	.reload = module_reload,
	.step = module_step,
	.unload = module_unload,
	.finalize = module_finalize
};
}
