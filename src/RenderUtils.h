#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#pragma once

#include "GLFW/glfw3.h"
#include <bgfx/bgfx.h>

struct RenderState;

namespace bgfxutils {

	bgfx::ShaderHandle loadShader(const char *_name);

	bgfx::ProgramHandle loadProgram(const char *_vsName, const char *_fsName);

	bgfx::ProgramHandle loadProgramFromFiles(const char *_vsFileName, const char *_fsFileName);

	bgfx::TextureHandle loadTexture(const char *_name, uint32_t _flags = BGFX_TEXTURE_NONE, uint8_t _skip = 0,
									bgfx::TextureInfo *_info = NULL);

	void calcTangents(void *_vertices, uint16_t _numVertices, bgfx::VertexDecl _decl, const uint16_t *_indices,
					  uint32_t _numIndices);

	struct MeshState {
		struct Texture {
			uint32_t m_flags;
			bgfx::UniformHandle m_sampler;
			bgfx::TextureHandle m_texture;
			uint8_t m_stage;
		};

		Texture m_textures[4];
		uint64_t m_state;
		bgfx::ProgramHandle m_program;
		uint8_t m_numTextures;
		uint8_t m_viewId;
	};

	struct Mesh;

	Mesh *meshLoad(const char *_filePath);

	void meshUnload(Mesh *_mesh);

	MeshState *meshStateCreate();

	void meshStateDestroy(MeshState *_meshState);

	void meshSubmit(const Mesh *_mesh, uint8_t _id, bgfx::ProgramHandle _program, const float *_mtx,
					uint64_t _state = BGFX_STATE_MASK);

	void meshSubmit(const Mesh *_mesh, const RenderState*_state, uint8_t _numPasses, const float *_mtx,
					uint16_t _numMatrices = 1);

	// Loads the mesh data from a VBO into a bgfx Mesh
//	Mesh *createMeshFromVBO (const MeshVBO& mesh_buffer);
}

#endif
