#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#pragma once

#include "GLFW/glfw3.h"
#include <bgfx/bgfx.h>

// Forward declarations
struct RenderState;

namespace bgfxutils {
struct Mesh;
}

struct Mesh {
	bgfxutils::Mesh* mBgfxMesh = nullptr;
 	std::vector<Vector4f> mVertices;
 	std::vector<Vector3f> mNormals;
 	std::vector<Vector4f> mColors;

	~Mesh();
	void Update();
	void Merge (const Mesh& other, const Matrix44f &mat = Matrix44f::Identity());
	void Submit (const RenderState *state, const float* matrix) const;

	static Mesh *sCreateCuboid (float width, float height, float depth);
	static Mesh *sCreateUVSphere (int rows, int segments, float radius = 1.0f);
	static Mesh *sCreateCylinder (int segments);
	static Mesh *sCreateCapsule (int rows, int segments, float length, float radius);
};

namespace bgfxutils {
	bgfx::ShaderHandle loadShader(const char *_name);

	bgfx::ProgramHandle loadProgram(const char *_vsName, const char *_fsName);

	bgfx::ProgramHandle loadProgramFromFiles(const char *_vsFileName, const char *_fsFileName);

	bgfx::TextureHandle loadTexture(const char *_name, uint32_t _flags = BGFX_TEXTURE_NONE, uint8_t _skip = 0,
									bgfx::TextureInfo *_info = NULL);

	void calcTangents(void *_vertices, uint16_t _numVertices, bgfx::VertexDecl _decl, const uint16_t *_indices,
					  uint32_t _numIndices);

	Mesh *meshLoad(const char *_filePath);

	void meshUnload(Mesh *_mesh);

	void meshSubmit(const Mesh *_mesh, uint8_t _id, bgfx::ProgramHandle _program, const float *_mtx,
					uint64_t _state = BGFX_STATE_MASK);

	void meshSubmit(const Mesh *_mesh, const RenderState*_state, uint8_t _numPasses, const float *_mtx,
					uint16_t _numMatrices = 1);

	// Loads the mesh data from a VBO into a bgfx Mesh
//	Mesh *createMeshFromVBO (const MeshVBO& mesh_buffer);

	void meshTransform (Mesh* mesh, const float *mtx);

}

#endif
