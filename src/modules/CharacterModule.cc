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
#include "LuaTableTypes.h"

#include "rbdl/addons/luamodel/luatables.h"

// forward declaration of a function that is in the LuaModel addon
// of RBDL. We make it visible here to extract more information
// from the Lua file.
namespace RigidBodyDynamics {
namespace Addons {
bool LuaModelReadFromTable (LuaTable &model_table, Model *model, bool verbose);
}
}

// Types needed to parse

using namespace std;
using namespace RigidBodyDynamics;
using namespace RigidBodyDynamics::Addons;

const float cJumpVelocity = 4.0f;
const float cVelocityDamping = 4.0f;
const float cGravity = 9.81f;
const float cGroundAcceleration = 30.0f;
const float cCharacterHeight = 1.8f;
const float cCharacterWidth = 1.f;
const char* cRigModelFile = "data/models/model.lua";

CharacterEntity::CharacterEntity() {
	mEntity = gRenderer->createEntity();
	mPosition = Vector3f (0.f, 0.0f, 0.0f);

	mRigModel = new Model();
	
	bool load_result = LoadRig (cRigModelFile);
	assert (load_result);

	cout << "Creating render entity mesh ..." << endl;

//	Mesh* base_mesh = Mesh::sCreateUVSphere(45, 45, 0.9);
//	Transform sub_transform =
//		Transform::fromTransRot(
//				Vector3f (4.f, 3.3f, 0.12f),
//				Quaternion::fromEulerYXZ (Vector3f(1.f, 2.f, 0.f))
//				);
//	Mesh* quad = Mesh::sCreateCuboid(1.05, 0.1, 2.05);
//
//	base_mesh->Merge (*quad, sub_transform.toMatrix());
//	base_mesh->Update();
//	delete quad;

//	int bone_index;
//
//	// Build the snowman
//
//	// bottom sphere
//	bone_index = mEntity->mSkeleton.AddBone(
//			-1, 
//			Transform::fromTrans(
//				Vector3f (0.0f, 0.9 * 0.5f, 0.0f)
//				)
//			);
//	mEntity->mSkeletonMeshes.AddMesh (
//			Mesh::sCreateUVSphere(45, 45, 0.9),
//			bone_index
//			);
//	gLog ("Now have %d bones, bone_index %d", 
//			mEntity->mSkeleton.Length(),
//			bone_index);
//
//	// middle sphere
//	bone_index = mEntity->mSkeleton.AddBone(
//			bone_index, 
//			Transform::fromTrans(
//				Vector3f (0.0f, 0.55f, 0.0f)
//				)
//			);
//	mEntity->mSkeletonMeshes.AddMesh (
//			Mesh::sCreateUVSphere (45, 45, 0.7),
//			bone_index
//			);
//	gLog ("Now have %d bones, bone_index %d", 
//			mEntity->mSkeleton.Length(),
//			bone_index);
//
//	// top sphere
//	bone_index = mEntity->mSkeleton.AddBone(
//			bone_index, 
//			Transform::fromTrans(
//				Vector3f (0.0f, 0.4f, 0.0f)
//				)
//			);
//	mEntity->mSkeletonMeshes.AddMesh (
//			Mesh::sCreateUVSphere (45, 45, 0.5),
//			bone_index
//			);

	//	mState->character->entity->mesh = bgfxutils::createCuboid (1.f, 1.f, 1.f);
	//	mState->character->entity->mesh = bgfxutils::createCylinder (20);
	cout << "Creating render entity mesh ... success!" << endl;
}

CharacterEntity::~CharacterEntity() {
	gRenderer->destroyEntity(mEntity);
	mEntity = nullptr;
	delete mRigModel;
	mRigModel = nullptr;
}

bool CharacterEntity::LoadRig(const char* filename) {
	gLog ("Creating rig model from %s ... ", filename);
	LuaTable model_table = LuaTable::fromFile (filename);

	bool load_result = LuaModelReadFromTable (model_table, mRigModel, false);

	gLog ("Creating rig model from %s ... %s", filename, load_result ? "success" : "failed!");
	gLog ("Rig model has %d degrees of freedom", mRigModel->qdot_size);

	gLog ("Reading rig geometry information ... ");

	int frame_count = model_table["frames"].length();
	gLog ("Found %d frames", frame_count);
	std::map<std::string, int> frame_name_bone_index;

	int num_meshes = 0;

	for (int fi = 1; fi <= frame_count; ++fi) {
		LuaTableNode frame_table = model_table["frames"][fi];
		string frame_name = frame_table["name"].getDefault<std::string>("ROOT");

		// create a bone for the frame
		string frame_parent = frame_table["parent"].getDefault<std::string>("ROOT");
		int parent_index = -1;
		if (frame_parent != "ROOT") {
			if (frame_name_bone_index.find(frame_parent) 
				== frame_name_bone_index.end()) {
				gLog ("  Error: could not find frame for parent %s",
						frame_parent.c_str());
			}

			parent_index = frame_name_bone_index[frame_parent];
		}

		// assemble the parent transform
		Transform parent_transform;
		if (frame_table["joint_frame"].exists()) {
			Vector3f translation = frame_table["joint_frame"]["r"].getDefault(Vector3f(0.f, 0.f, 0.f));
			Matrix33f rot_matrix = frame_table["joint_frame"]["E"].getDefault(
					Matrix33f::Identity()
					);
			parent_transform.translation = translation;
			parent_transform.rotation = Quaternion::fromMatrix(rot_matrix);
		}
		int bone_index = mEntity->mSkeleton.AddBone (
				parent_index,
				parent_transform
				);
		frame_name_bone_index[frame_name] = bone_index;
		gLog ("  Added frame %s, bone index %d",
				frame_name.c_str(),
				bone_index);

		// add visuals to the bone
		int visuals_count = frame_table["visuals"].length();
		gLog ("  Frame %s has %d visuals", frame_name.c_str(), visuals_count);
		for (int vi = 1; vi <= visuals_count; ++vi) {
			LuaTableNode visual_table = frame_table["visuals"][vi];
			bool have_geometry = visual_table["geometry"].exists();

			if (!have_geometry) {
				gLog ("  Warning: could not find geometry for visual %d of frame %s",
						vi, frame_name.c_str());
				continue;
			}

			Vector4f color (1.f, 0.f, 1.f, 1.f);
			if (visual_table["color"].length() == 3) {
				color.block<3,1>(0,0) = visual_table["color"].getDefault(Vector3f(color.block<3,1>(0,0)));
			} if (visual_table["color"].length() == 4) {
				color = visual_table["color"].getDefault(color);
			}

			// read the geometry
			Mesh* mesh = nullptr;

			if (visual_table["geometry"]["box"].exists()) {
				Vector3f dimensions = visual_table["geometry"]["box"]["dimensions"].getDefault(Vector3f (1.f, 1.f, 1.f));
				mesh = Mesh::sCreateCuboid (
						dimensions[0],
						dimensions[1],
						dimensions[2]
						);
			} else if (visual_table["geometry"]["sphere"].exists()) {
				int rows = static_cast<int>(
						visual_table["geometry"]["sphere"]["rows"].getDefault (16.f)
						);				
				int segments = static_cast<int>(
						visual_table["geometry"]["sphere"]["segments"].getDefault (16.f)
						);
				float radius = visual_table["geometry"]["sphere"]["radius"].getDefault (1.f);
				mesh = Mesh::sCreateUVSphere (rows, segments, radius);
			} else if (visual_table["geometry"]["capsule"].exists()) {
				int rows = static_cast<int>(
						visual_table["geometry"]["capsule"]["radius"].getDefault (16.f)
						);				
				int segments = static_cast<int>(
						visual_table["geometry"]["capsule"]["segments"].getDefault (16.f)
						);
				float radius = visual_table["geometry"]["capsule"]["radius"].getDefault (1.f);
				float length = visual_table["geometry"]["capsule"]["length"].getDefault (1.f);
				mesh = Mesh::sCreateCapsule (rows, segments, length, radius);
			}

			if (mesh == nullptr) {
				gLog ("  Warning: could not find geometry for visual %d of frame %s",
						vi, frame_name.c_str());
				continue;
			}
			
			if (mesh->mVertices.size() == 0) {
				gLog ("  Warning: Invalid geometry for visual %d of frame %s: no vertices found",
						vi, frame_name.c_str());
			}




			Transform mesh_transform;
			if (visual_table["scale"].exists()) {
				gLog("  Warning: keyword scale not supported for visual %d of frame %s",
						vi, frame_name.c_str());
			}

			Vector3f dimensions = visual_table["dimensions"].getDefault(Vector3f(0.f, 0.f, 0.f));
			Vector3f mesh_center = visual_table["mesh_center"].getDefault(Vector3f(0.f, 0.f, 0.f));
			Vector3f translate = visual_table["translate"].getDefault(Vector3f(0.f, 0.f, 0.f));
			
			mesh->UpdateBounds();

			Vector3f bbox_size = mesh->mBoundsMax - mesh->mBoundsMin;
			mesh_transform.scale = Vector3f( 	
							fabs(dimensions[0]) / bbox_size[0],
							fabs(dimensions[1]) / bbox_size[1],
							fabs(dimensions[2]) / bbox_size[2]
								);
			mesh->Transform (mesh_transform.toMatrix());
			mesh->UpdateBounds();
			mesh->Update();

			mEntity->mSkeletonMeshes.AddMesh(
					mesh,
					bone_index
					);
			num_meshes++;
		}
	}

	gLog ("Loaded rig with %d bones and %d meshes", 
			mEntity->mSkeleton.Length(),
			num_meshes);

	return load_result;
}

static float cur_time = 0.0f;

void CharacterEntity::Update(float dt) {
	Vector3f mController_acceleration (
			mController.mDirection[0] * cGroundAcceleration,
			mController.mDirection[1] * cGroundAcceleration,
			mController.mDirection[2] * cGroundAcceleration
			);

	Vector3f gravity (0.0f, -cGravity, 0.0f);
	Vector3f damping (
			-mVelocity[0] * cVelocityDamping,
			0.0f,
			-mVelocity[2] * cVelocityDamping
			);

	Vector3f acceleration = mController_acceleration + gravity + damping;

	mVelocity = mVelocity + acceleration * dt;

	if (mPosition[1] == 0.0f 
			&& mController.mState[CharacterController::ControlStateJump]) {
		mVelocity[1] = cJumpVelocity;	
	}

	// integrate mPosition
	mPosition += mVelocity * dt;

	if (mPosition[1] < 0.f) {
		mPosition[1] = 0.f;
		mVelocity[1] = 0.0f;
	}

	// apply transformation
	mEntity->mTransform.translation.set(
			mPosition[0],
			mPosition[1],
			mPosition[2]);

	gRenderer->drawDebugSphere (Vector3f (0.f, 1.3 + sin(cur_time * 2.f), 0.f), 2.2f);


	Quaternion quat (-cos(cur_time), 0.0f, 0.0f * sin(cur_time), 1.0f);
	quat.normalize();

	mEntity->mTransform.rotation = quat;

	// update matrices
	mEntity->mSkeleton.UpdateMatrices(mEntity->mTransform.toMatrix());

	cur_time += dt;
}

void ShowCharacterPropertiesWindow (CharacterEntity* character) {
	assert (character != nullptr);
	ImGui::SetNextWindowSize (ImVec2(600.f, 300.0f), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos (ImVec2(400.f, 16.0f), ImGuiSetCond_Once);
	ImGui::Begin("Character");

	if (ImGui::Button ("Reset")) {
		character->Reset();
	}
	
	ImGui::DragFloat3 ("Position", character->mPosition.data(), 0.01, -10.0f, 10.0f);
	ImGui::DragFloat3 ("Velocity", character->mVelocity.data(), 0.01, -10.0f, 10.0f);

	for (int i = 0; i < character->mEntity->mSkeleton.Length(); ++i) {
		char buf[32];
		snprintf (buf, 32, "Mesh %d", i);

		ImGuiTreeNodeFlags node_flags = 0;

		bool node_open = ImGui::TreeNodeEx(
				buf,
				node_flags);

		if (node_open) {
			Transform &transform = character->mEntity->mSkeleton.mLocalTransforms[i];

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
//	SerializeVec3(*serializer, "protot.TestModule.entity.mPosition", state->character->mPosition);
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
