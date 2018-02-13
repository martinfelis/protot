#include "RuntimeModule.h"
#include "Globals.h"
#include "RenderModule.h"

struct Renderer;

static const GLfloat g_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
};

//
// Module
//
struct module_state {
	Renderer *renderer;
};

static struct module_state *module_init() {
	gLog ("%s %s called", __FILE__, __FUNCTION__);
	assert (gWindow != nullptr && "Cannot initialize renderer module without gWindow!");

	module_state *state = (module_state*) malloc(sizeof(*state));
	state->renderer = new Renderer();
	assert (state->renderer != nullptr);

	return state;
}

template <typename Serializer>
static void module_serialize (
		struct module_state *state,
		Serializer* serializer) {
//	// get the state from the serializer
//	Camera* camera = &gRenderer->cameras[gRenderer->activeCameraIndex];
//	assert (camera != nullptr);

//	SerializeBool (*serializer, "protot.RenderModule.draw_floor", gRenderer->drawFloor);
//	SerializeBool (*serializer, "protot.RenderModule.draw_skybox", gRenderer->drawSkybox);
//	SerializeBool (*serializer, "protot.RenderModule.debug_enabled", gRenderer->drawDebug);
//	SerializeVec3 (*serializer, "protot.RenderModule.camera.eye", camera->eye);
//	SerializeVec3 (*serializer, "protot.RenderModule.camera.poi", camera->poi);
}

static void module_finalize(struct module_state *state) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);

	assert (state->renderer != nullptr);
	delete state->renderer;

	free(state);
}

static void module_reload(struct module_state *state, void *read_serializer) {
	gLog ("%s %s called (state %p)", __FILE__, __FUNCTION__, state);
	assert (gWindow != nullptr);

	gLog ("Renderer initialize");
	assert (state != nullptr);
	state->renderer->Initialize(100, 100);

	gRenderer = state->renderer;

	// load the state of the module
	if (read_serializer != nullptr) {
		module_serialize(state, static_cast<ReadSerializer*>(read_serializer));
	}
}

static void module_unload(struct module_state *state, void* write_serializer) {
	// serialize the state of the module
	if (write_serializer != nullptr) {
		module_serialize(state, static_cast<WriteSerializer*>(write_serializer));
	}

	gRenderer = nullptr;
	state->renderer->Shutdown();

	gLog ("RenderModule unload called");
}

static bool module_step(struct module_state *state, float dt) {
	int width, height;
	assert (gWindow != nullptr);
	state->renderer->RenderGui();
	state->renderer->RenderGl();

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



//
// Camera
//
void Camera::updateMatrices() {
}


//
// Camera
//
void Renderer::Initialize(int width, int height) {
	glGenVertexArrays(1, &mMesh.mVertexArrayId);
	glBindVertexArray(mMesh.mVertexArrayId);
	glGenBuffers(1, &mMesh.mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mMesh.mVertexBuffer);
glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	mProgram = RenderProgram("data/shaders/vs_simple.glsl", "data/shaders/fs_simple.glsl");
	bool load_result = mProgram.Load();
	assert(load_result);
}

void Renderer::Shutdown() {
	glDeleteVertexArrays(1, &mMesh.mVertexArrayId);
}
void Renderer::RenderGl() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(mProgram.mProgramId);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mMesh.mVertexBuffer);
	glVertexAttribPointer(
			0,				// attribute 0
			3,				// size
			GL_FLOAT,	// type
			GL_FALSE,	// normalized?
			0,				// stride
			(void*)0	// offset
			);

	glDrawArrays(GL_TRIANGLES, 0, 3);	// starting from vertex 0; 3 vertices total
	glDisableVertexAttribArray(0);
}

void Renderer::RenderGui() {
}

void Renderer::Resize (int width, int height) {
	assert(false);
}
