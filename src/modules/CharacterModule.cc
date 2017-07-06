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
#include "imgui_dock.h"
#include "Serializer.h"
#include "Timer.h"

#include "string_utils.h"

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
using namespace RigidBodyDynamics;
using namespace RigidBodyDynamics::Addons;

const float cJumpVelocity = 4.0f;
const float cVelocityDamping = 4.0f;
const float cGravity = 9.81f;
const float cGroundAcceleration = 10.0f;
const float cCharacterHeight = 1.8f;
const float cCharacterWidth = 1.f;
const char* cRigModelFile = "data/models/model.lua";
const char* cAnimFile = "data/models/anim.csv";
static bool sCharacterDock = true;

static VectorNf sRigQ;
Quaternion offset_quat;

struct module_state {
};

bool Animation::Load(const char* filename) {
	ifstream infile (filename);
	if (!infile) {
		cerr << "Error reading animation file from '" << filename << "'" << endl;
		abort();
		return false;
	}

	mFrames.clear();

	int line_index = 0;
	string line;
	string previous_line;

	while (!infile.eof()) {
		previous_line = line;
		getline (infile, line);

		if (infile.eof()) {
			break;
		} else {
			line_index++;
		}

		vector<string> tokens = tokenize_csv_strip_whitespaces (line);
		if (tokens.size() == 0)
			continue;

		double frame_time = 0.;
		VectorNf state = VectorNf::Zero (tokens.size() - 1);

		double value;
		istringstream value_stream (tokens[0]);

		// If the first entry is not a number we ignore the whole line
		if (!(value_stream >> value))
			continue;

		for (size_t i = 0; i < tokens.size(); i++) {
			value_stream.clear();
			value_stream.str(tokens[i]);
			if (!(value_stream >> value)) {
				cerr << "Error: could not convert string '" << tokens[i] << "' to number in " << filename << ", line " << line_index << ", column " << i << endl;
				abort();
			}

			if (i == 0)
				frame_time = value;
			else {
				state[i - 1] = value;
			}
		}
		mFrameTimes.push_back(frame_time);
		mDuration = frame_time;
		mFrames.push_back(state);
	}

	infile.close();

	return true;
}

void Animation::Sample(float cur_time, VectorNf& state) {
	assert (mFrameTimes.size() > 0 && mFrameTimes[0] == 0.0f);

	while (mIsLooped && cur_time >= mDuration) {
		cur_time -= mDuration;
	}
	assert(cur_time >= 0.0f);

	int next = 0;
	for (next = 0; next < mFrameTimes.size() - 1; ++next) {
		if (mFrameTimes[next] > cur_time)
			break;
	}
	float t1 = mFrameTimes[next];
	assert (t1 > cur_time);

	int prev = (next - 1) % mFrameTimes.size();
	if (prev < 0)
		prev += mFrameTimes.size();
	float t0 = mFrameTimes[prev];

	// if we do not want to loop we use the last frame
	assert (cur_time >= t0 && cur_time <= t1);
	float alpha = (cur_time - t0) / (t1 - t0);
	state = mFrames[prev] * (1.0f - alpha) + mFrames[next] * alpha;
}

CharacterEntity::CharacterEntity() {
	mEntity = gRenderer->createEntity();
	mPosition = Vector3f (0.f, 0.0f, 0.0f);

	mRigModel = new Model();
	
	bool load_result = LoadRig (cRigModelFile);
	assert (load_result);

	cout << "Creating render entity mesh ..." << endl;

	//	mState->character->entity->mesh = bgfxutils::createCuboid (1.f, 1.f, 1.f);
	//	mState->character->entity->mesh = bgfxutils::createCylinder (20);
	cout << "Creating render entity mesh ... success!" << endl;

	gLog ("Loading Animation %s", cAnimFile);
	load_result = mAnimation.Load(cAnimFile);
	assert (load_result);

	gLog ("Initializing IK constraints");
	IKConstraint constraint;
	constraint.mEffectorBodyId = mRigModel->GetBodyId("FootRight");
	mIKConstraints.push_back(constraint);

	mAnimTime = 0.0f;
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

	mRigModel->fixed_body_discriminator = 1000;

	bool load_result = LuaModelReadFromTable (model_table, mRigModel, false);

	gLog ("Creating rig model from %s ... %s", filename, load_result ? "success" : "failed!");
	gLog ("Rig model has %d degrees of freedom", mRigModel->qdot_size);
	mRigState.q = VectorNd::Zero (mRigModel->q_size);

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
		gLog ("  Added frame %s, bone index %d, parent index %d",
				frame_name.c_str(),
				bone_index,
				parent_index);
		assert (bone_index == mBoneFrameIndices.size());
		mBoneFrameIndices.push_back(mRigModel->GetBodyId(frame_name.c_str()));

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

			// parse mesh rotation
			float angle = 0.0f;
			Vector3f axis (1.f, 0.f, 0.f);
			if (visual_table["rotate"].exists()) {
				angle = visual_table["rotate"]["angle"].getDefault (0.0f);
				axis = visual_table["rotate"]["axis"].getDefault (axis);
			}

			Quaternion visual_rotate (Quaternion::fromAxisAngle (axis, angle * M_PI / 180.f));
			mesh_transform.rotation = visual_rotate;

			// compute mesh center
			mesh->UpdateBounds();

			Vector3f bbox_size = mesh->mBoundsMax - mesh->mBoundsMin;
			mesh_transform.scale = Vector3f( 	
							fabs(dimensions[0]) / bbox_size[0],
							fabs(dimensions[1]) / bbox_size[1],
							fabs(dimensions[2]) / bbox_size[2]
								);
			if (translate.squaredNorm() > 0.0f) {
				mesh_transform.translation = translate;
			} else {
				mesh_transform.translation = mesh_center;
			}

			// apply transform
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

void CharacterEntity::ApplyCharacterController(float dt) {
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

	mPosition = mEntity->mTransform.translation;

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

	// Convert to different coordinate frame
	Quaternion quat (1., 0.0f, 0.0f, 1.0f);
	quat.normalize();

	float plane_angle = atan2 (mVelocity[2], mVelocity[0]);
	Quaternion heading_rot (Quaternion::fromAxisAngle(Vector3f (0.f, 1.f, 0.f), plane_angle));

	mEntity->mTransform.rotation = quat * heading_rot;

	if (mVelocity.squaredNorm() > 0.01f) {
		mAnimTime += dt;
		VectorNf sampled_anim_state;
		mAnimation.Sample(mAnimTime, sampled_anim_state);
		mRigState.q = sampled_anim_state;
	} else {
		mAnimTime = mAnimation.mDuration * 0.125f;
		mRigState.q.setZero();
	}
}

void CharacterEntity::UpdateIKGizmos() {
	for (int i = 0; i < mIKConstraints.size(); ++i) {
		IKConstraint& constraint = mIKConstraints[i];
		Transform ik_handle_transform;
		ik_handle_transform.translation = constraint.mEffectorWorldTarget;
		Matrix44f ent_transform = ik_handle_transform.toMatrix(); 

		Camera *active_camera = &gRenderer->cameras[gRenderer->activeCameraIndex];

		ImGuizmo::Manipulate(
				active_camera->mtxView,
				active_camera->mtxProj,
				ImGuizmo::TRANSLATE,
				ImGuizmo::LOCAL,
				ent_transform.data()
				);

		ik_handle_transform.fromMatrix(ent_transform);
		constraint.mEffectorWorldTarget = ik_handle_transform.translation;
	}
}

void CharacterEntity::UpdateIKConstraintSet() {
	if (mIKConstraints.size() * 3 != mIKConstraintSet.num_constraints) {
		mIKConstraintSet.ClearConstraints();
		for (int i = 0; i < mIKConstraints.size(); ++i) {
			const IKConstraint& constraint = mIKConstraints[i];
			mIKConstraintSet.AddPointConstraint(
					constraint.mEffectorBodyId,
					constraint.mEffectorLocalOffset,
					constraint.mEffectorWorldTarget
					);
		}

		mIKConstraintSet.lambda = 1.0e-3;
		mIKConstraintSet.max_steps = 2;
		mIKConstraintSet.step_tol = 1.0e-5;
	} else {
		for (int i = 0; i < mIKConstraints.size(); ++i) {
			const IKConstraint& constraint = mIKConstraints[i];
			mIKConstraintSet.body_ids[i] = constraint.mEffectorBodyId;
			mIKConstraintSet.body_points[i] = constraint.mEffectorLocalOffset;
			mIKConstraintSet.target_positions[i] = Vector3d (
					constraint.mEffectorWorldTarget[0],
					-constraint.mEffectorWorldTarget[2],
					constraint.mEffectorWorldTarget[1]
					);
		}
	}
}

void CharacterEntity::ApplyIKConstraints() {
	if (mIKConstraints.size() == 0)
		return;

	UpdateIKConstraintSet();

	bool result = false;
	VectorNd q_init (mRigState.q);
	VectorNd q_res (mRigState.q);
	result = RigidBodyDynamics::InverseKinematics(
			*mRigModel,
			q_init,
			mIKConstraintSet,
			q_res
			);
	mRigState.q = q_res;

	for (int i = 0; i < mIKConstraints.size(); ++i) {
		const IKConstraint& constraint = mIKConstraints[i];
		UpdateKinematicsCustom (
				*mRigModel, &q_res, nullptr, nullptr);

		Vector3f effector_pos = CalcBodyToBaseCoordinates(
				*mRigModel, 
				q_res, 
				constraint.mEffectorBodyId, 
				constraint.mEffectorLocalOffset,
				false
				);

		gRenderer->drawDebugSphere (
				effector_pos,
				0.05f,
				Vector4f (0.f, 1.f, 0.f, 1.0f)
				);

		gRenderer->drawDebugSphere (
				constraint.mEffectorWorldTarget,
				0.05f,
				Vector4f (1.f, 0.f, 0.f, 1.0f)
				);

		gRenderer->drawDebugLine (
				effector_pos,
				constraint.mEffectorWorldTarget,
				Vector3f (1.f, 0.f, 1.f)
				);
	}
}

void CharacterEntity::Update(float dt) {
	UpdateIKGizmos();
	ApplyCharacterController(dt);
//	ApplyIKConstraints();
	UpdateBoneMatrices();

	cur_time += dt;
}

void CharacterEntity::UpdateBoneMatrices() {
	VectorNd q = mRigState.q;
	UpdateKinematicsCustom(*mRigModel, &q, nullptr, nullptr);

	for (int i = 0; i < mBoneFrameIndices.size(); ++i) {
		int frame_index = mBoneFrameIndices[i];

		if (frame_index < mRigModel->fixed_body_discriminator)
		{
			Matrix33f mat = mRigModel->X_lambda[frame_index].E;
			mEntity->mSkeleton.mLocalTransforms[i].rotation =
				Quaternion::fromMatrix(mat);
//			mEntity->mSkeleton.mLocalTransforms[i].translation =
//				mRigModel->X_lambda[frame_index].r;
		} else {
			const FixedBody& fbody = 
				mRigModel->mFixedBodies[frame_index - mRigModel->fixed_body_discriminator];

			Math::SpatialTransform parent_transform; // = mRigModel->X_lambda[fbody.mMovableParent];
			Math::SpatialTransform fixed_transform = fbody.mParentTransform;

			Math::SpatialTransform transform = fixed_transform * parent_transform;

			Matrix33f mat = transform.E;
			mEntity->mSkeleton.mLocalTransforms[i].rotation =
				Quaternion::fromMatrix(mat);
//			mEntity->mSkeleton.mLocalTransforms[i].translation =
//				transform.r;
		}
	}

	// update matrices
	Transform entity_rig_transform = mEntity->mTransform;
	entity_rig_transform.translation[1] += 0.98f;
	mEntity->mSkeleton.UpdateMatrices(entity_rig_transform.toMatrix());
}

void ShowCharacterPropertiesWindow (CharacterEntity* character) {
	assert (character != nullptr);
	ImGui::SetNextWindowSize (ImVec2(600.f, 300.0f), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos (ImVec2(400.f, 16.0f), ImGuiSetCond_Once);

	if (ImGui::BeginDock("Character", &sCharacterDock)) {

		if (ImGui::Button ("Reset")) {
			character->Reset();
		}

		ImGui::Checkbox("Render", &character->mEntity->mDrawEntity);

		ImGui::Protot::DragFloat4Normalized ("Offset Quat", offset_quat.data(),
				0.01f, -1.0f, 1.0f);

		ImGui::DragFloat3 ("Position", character->mPosition.data(), 0.01, -10.0f, 10.0f);
		ImGui::DragFloat3 ("Velocity", character->mVelocity.data(), 0.01, -10.0f, 10.0f);
		float angle = atan2 (-character->mVelocity[2], character->mVelocity[0]);
		ImGui::LabelText ("", "Angle %f", angle * 180.f / M_PI);

		ImGui::LabelText("", 
				"Skeleton Bones %d", 
				character->mEntity->mSkeleton.mLocalTransforms.size());

		ImGui::LabelText("", 
				"Rig Frames %d", 
				character->mRigModel->mBodies.size());

		bool node_open = ImGui::TreeNodeEx(
				"DOFs",
				0);
		if (node_open) {
			bool dof_modified = false;
			for (int i = 0; i < character->mRigModel->q_size; ++i) {
				char buf[32];
				snprintf (buf, 32, "DOF %d", i);

				if (ImGui::DragFloat (buf, &character->mRigState.q[i], 0.01, -10.0f, 10.0f)) {
					dof_modified = true;
				}
			}

			if (dof_modified) {
				VectorNd q = character->mRigState.q;
			}

			ImGui::TreePop();
		}

		node_open = ImGui::TreeNodeEx(
				"Meshes",
				0);

		if (node_open) {
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
			ImGui::TreePop();
		}

		node_open = ImGui::TreeNodeEx(
				"IK Effectors",
				0);

		if (node_open) {
			for (int i = 0; i < character->mIKConstraints.size(); ++i) {
				IKConstraint& constraint = character->mIKConstraints[i];

				char buf[32];
				snprintf (buf, 32, "Constraint %d", i);

				ImGuiTreeNodeFlags node_flags = 0;

				bool node_open = ImGui::TreeNodeEx(
						buf,
						node_flags);

				if (node_open) {
					Transform &transform = character->mEntity->mSkeleton.mLocalTransforms[i];

					ImGui::DragFloat3 ("Local Offset", constraint.mEffectorLocalOffset.data(), 0.01, 0.001f, 10.0f);
					ImGui::DragFloat3 ("Target", constraint.mEffectorWorldTarget.data(), 0.01, -10.0f, 10.0f);

					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::EndDock();
}

static struct module_state *module_init() {
	std::cout << "Module init called" << std::endl;
	module_state *state = (module_state*) malloc(sizeof(*state));
	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
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
