#include "GLFW/glfw3.h"

#include <string.h> // strlen
#include <locale.h>

#include <iostream>
#include "common.h"

#include <tinystl/allocator.h>
#include <tinystl/vector.h>
#include <tinystl/string.h>
namespace stl = tinystl;

#include <bx/readerwriter.h>
#include <bx/fpumath.h>
#include <bx/string.h>
#include <bx/crtimpl.h>
#include "entry/dbg.h"
#include <ib-compress/indexbufferdecompression.h>

#include "shaderc.h"

#include "Renderer.h"
#include "RenderUtils.h"
//#include "MeshVBO.h"

void *load(const char *_filePath, uint32_t *_size = NULL);

void unload(void *_ptr);


namespace bgfx
{
//	int32_t read(bx::ReaderI* _reader, bgfx::VertexDecl& _decl);
//	int32_t read(bx::ReaderI* _reader, bgfx::VertexDecl& _decl, bx::Error* _err = NULL);
}

namespace bgfxutils {

namespace entry {
	static bx::FileReaderI* s_fileReader = NULL;
	static bx::FileWriterI* s_fileWriter = NULL;

	bx::AllocatorI* getDefaultAllocator()
	{
		BX_PRAGMA_DIAGNOSTIC_PUSH();
		BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
		BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
		static bx::CrtAllocator s_allocator;
		return &s_allocator;
		BX_PRAGMA_DIAGNOSTIC_POP();
	}
	static bx::AllocatorI* s_allocator = getDefaultAllocator();

	bx::FileReaderI* getFileReader()
	{
		if (s_fileReader == NULL) {
			s_fileReader = new bx::CrtFileReader;
		}
		return s_fileReader;
	}

	bx::FileWriterI* getFileWriter()
	{
		if (s_fileWriter == NULL) {
			s_fileWriter = new bx::CrtFileWriter;
		}
		return s_fileWriter;
	}

	bx::AllocatorI* getAllocator()
	{
		return s_allocator;
	}
}

void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size)
{
	if (0 == bx::open(_reader, _filePath) )
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void* data = BX_ALLOC(_allocator, size);
		bx::read(_reader, data, size);
		bx::close(_reader);
		if (NULL != _size)
		{
			*_size = size;
		}
		return data;
	}
	else
	{
		DBG("Failed to open: %s.", _filePath);
	}

	if (NULL != _size)
	{
		*_size = 0;
	}
	return NULL;
}

void* load(const char* _filePath, uint32_t* _size)
{
	return load(entry::getFileReader(), entry::getAllocator(), _filePath, _size);
}

void unload(void* _ptr)
{
	BX_FREE(entry::getAllocator(), _ptr);
}

static const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath)
{
	if (0 == bx::open(_reader, _filePath) )
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		const bgfx::Memory* mem = bgfx::alloc(size+1);
		bx::read(_reader, mem->data, size);
		bx::close(_reader);
		mem->data[mem->size-1] = '\0';
		return mem;
	} else {
		std::cerr << "Error opening file " << _filePath << std::endl;
		abort();
	}

	return NULL;
}

static void* loadMem(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size)
{
	if (0 == bx::open(_reader, _filePath) )
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void* data = BX_ALLOC(_allocator, size);
		bx::read(_reader, data, size);
		bx::close(_reader);

		if (NULL != _size)
		{
			*_size = size;
		}
		return data;
	}

	DBG("Failed to load %s.", _filePath);
	return NULL;
}

static bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name)
{
	char filePath[512];

	const char* shaderPath = "shaders/dx9/";

	switch (bgfx::getRendererType() )
	{
	case bgfx::RendererType::Direct3D11:
	case bgfx::RendererType::Direct3D12:
		shaderPath = "shaders/dx11/";
		break;

	case bgfx::RendererType::OpenGL:
		shaderPath = "shaders/glsl/";
		break;

	case bgfx::RendererType::Metal:
		shaderPath = "shaders/metal/";
		break;

	case bgfx::RendererType::OpenGLES:
		shaderPath = "shaders/gles/";
		break;

	default:
		break;
	}

	strcpy(filePath, shaderPath);
	strcat(filePath, _name);
	strcat(filePath, ".bin");

	return bgfx::createShader(loadMem(_reader, filePath) );
}

bgfx::ShaderHandle loadShader(const char* _name)
{
	return loadShader(entry::getFileReader(), _name);
}

bgfx::ProgramHandle loadProgram(bx::FileReaderI* _reader, const char* _vsName, const char* _fsName)
{
	bgfx::ShaderHandle vsh = loadShader(_reader, _vsName);
	bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
	if (NULL != _fsName)
	{
		fsh = loadShader(_reader, _fsName);
	}

	return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}

bgfx::ProgramHandle loadProgram(const char* _vsName, const char* _fsName)
{
	return loadProgram(entry::getFileReader(), _vsName, _fsName);
}

bgfx::ProgramHandle loadProgramFromFiles(const char *_vsFileName, const char *_fsFileName) {
	const char* argv[11];
	argv[0] = "--type";
	argv[1] = "vertex";
	argv[2] = "--platform";
	argv[3] = "linux";
	argv[4] = "-i";
	argv[5] = "shaders/common";
	argv[6] = "--varyingdef";
	argv[7] = "-p";
	argv[8] = "120";
	argv[9] = "-f";
	argv[10] = _vsFileName;
	
	bx::CommandLine cmdLine (11, argv);

	const bgfx::Memory* vs_source_memory = loadMem(entry::getFileReader(), _vsFileName);
	bx::ReaderSeekerI* vs_source_reader = new bx::MemoryReader (vs_source_memory->data, vs_source_memory->size);

	bx::MemoryBlock* vs_compiled_memory = new bx::MemoryBlock (entry::getAllocator());
	bx::MemoryWriter* vs_compiled_writer = new bx::MemoryWriter (vs_compiled_memory);

	setlocale(LC_NUMERIC, "C");
	int result = compileShader (cmdLine, vs_source_reader, vs_compiled_writer);
	if (result != EXIT_SUCCESS) {
		std::cerr << "Error compiling shader " << _vsFileName << std::endl;
		abort();
	}

	uint32_t size = vs_compiled_writer->seek(0, bx::Whence::End);
	const bgfx::Memory* mem = bgfx::alloc(size+1);
	bx::ReaderSeekerI* vs_compiled_reader = new bx::MemoryReader ((uint8_t*) vs_compiled_memory->more(), size);
	bx::read(vs_compiled_reader, mem->data, size);
	delete vs_compiled_reader;
	mem->data[mem->size-1] = '\0';
	bgfx::ShaderHandle vsh = bgfx::createShader(mem);

	delete vs_source_reader;
	delete vs_compiled_writer;
	delete vs_compiled_memory;

	bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;

	if (_fsFileName != NULL) {
		argv[1] = "fragment";

		const bgfx::Memory* fs_source_memory = loadMem(entry::getFileReader(), _fsFileName);
		bx::ReaderSeekerI* fs_source_reader = new bx::MemoryReader (fs_source_memory->data, fs_source_memory->size);

		bx::MemoryBlock* fs_compiled_memory = new bx::MemoryBlock (entry::getAllocator());
		bx::MemoryWriter* fs_compiled_writer = new bx::MemoryWriter (fs_compiled_memory);

		result = compileShader (cmdLine, fs_source_reader, fs_compiled_writer);
		if (result != EXIT_SUCCESS) {
			std::cerr << "Error compiling shader " << _fsFileName << std::endl;
			abort();
		}

		uint32_t size = fs_compiled_writer->seek(0, bx::Whence::End);
		const bgfx::Memory* mem = bgfx::alloc(size+1);
		bx::ReaderSeekerI* fs_compiled_reader = new bx::MemoryReader ((uint8_t*) fs_compiled_memory->more(), size);
		bx::read(fs_compiled_reader, mem->data, size);
		delete fs_compiled_reader;
		mem->data[mem->size-1] = '\0';
		fsh = bgfx::createShader(mem);

		delete fs_source_reader;
		delete fs_compiled_writer;
		delete fs_compiled_memory;
	}

	return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}

typedef unsigned char stbi_uc;
extern "C" stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);

bgfx::TextureHandle loadTexture(bx::FileReaderI* _reader, const char* _name, uint32_t _flags, uint8_t _skip, bgfx::TextureInfo* _info)
{
	char filePath[512] = { '\0' };
	if (NULL == strchr(_name, '/') )
	{
		strcpy(filePath, "textures/");
	}

	strcat(filePath, _name);

	if (NULL != bx::stristr(_name, ".dds")
	||  NULL != bx::stristr(_name, ".pvr")
	||  NULL != bx::stristr(_name, ".ktx") )
	{
		const bgfx::Memory* mem = loadMem(_reader, filePath);
		if (NULL != mem)
		{
			return bgfx::createTexture(mem, _flags, _skip, _info);
		}

		bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
		DBG("Failed to load %s.", filePath);
		return handle;
	}

	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	bx::AllocatorI* allocator = entry::getAllocator();

	uint32_t size = 0;
	void* data = loadMem(_reader, allocator, filePath, &size);
	if (NULL != data)
	{
		int width  = 0;
		int height = 0;
		int comp   = 0;

		uint8_t* img = NULL;
		img = stbi_load_from_memory( (uint8_t*)data, size, &width, &height, &comp, 4);

		BX_FREE(allocator, data);

		if (NULL != img)
		{
			handle = bgfx::createTexture2D(uint16_t(width), uint16_t(height), 
											false,
											1,
											bgfx::TextureFormat::RGBA8,
											_flags,
											bgfx::copy(img, width*height*4)
											);

			free(img);

			if (NULL != _info)
			{
				bgfx::calcTextureSize(*_info
					, uint16_t(width)
					, uint16_t(height)
					, 0
					, false
					, false
					, 1
					, bgfx::TextureFormat::RGBA8
					);
			}
		}
	}
	else
	{
		DBG("Failed to load %s.", filePath);
	}

	return handle;
}

bgfx::TextureHandle loadTexture(const char* _name, uint32_t _flags, uint8_t _skip, bgfx::TextureInfo* _info)
{
	return loadTexture(entry::getFileReader(), _name, _flags, _skip, _info);
}

void calcTangents(void* _vertices, uint16_t _numVertices, bgfx::VertexDecl _decl, const uint16_t* _indices, uint32_t _numIndices)
{
	struct PosTexcoord
	{
		float m_x;
		float m_y;
		float m_z;
		float m_pad0;
		float m_u;
		float m_v;
		float m_pad1;
		float m_pad2;
	};

	float* tangents = new float[6*_numVertices];
	memset(tangents, 0, 6*_numVertices*sizeof(float) );

	PosTexcoord v0;
	PosTexcoord v1;
	PosTexcoord v2;

	for (uint32_t ii = 0, num = _numIndices/3; ii < num; ++ii)
	{
		const uint16_t* indices = &_indices[ii*3];
		uint32_t i0 = indices[0];
		uint32_t i1 = indices[1];
		uint32_t i2 = indices[2];

		bgfx::vertexUnpack(&v0.m_x, bgfx::Attrib::Position,  _decl, _vertices, i0);
		bgfx::vertexUnpack(&v0.m_u, bgfx::Attrib::TexCoord0, _decl, _vertices, i0);

		bgfx::vertexUnpack(&v1.m_x, bgfx::Attrib::Position,  _decl, _vertices, i1);
		bgfx::vertexUnpack(&v1.m_u, bgfx::Attrib::TexCoord0, _decl, _vertices, i1);

		bgfx::vertexUnpack(&v2.m_x, bgfx::Attrib::Position,  _decl, _vertices, i2);
		bgfx::vertexUnpack(&v2.m_u, bgfx::Attrib::TexCoord0, _decl, _vertices, i2);

		const float bax = v1.m_x - v0.m_x;
		const float bay = v1.m_y - v0.m_y;
		const float baz = v1.m_z - v0.m_z;
		const float bau = v1.m_u - v0.m_u;
		const float bav = v1.m_v - v0.m_v;

		const float cax = v2.m_x - v0.m_x;
		const float cay = v2.m_y - v0.m_y;
		const float caz = v2.m_z - v0.m_z;
		const float cau = v2.m_u - v0.m_u;
		const float cav = v2.m_v - v0.m_v;

		const float det = (bau * cav - bav * cau);
		const float invDet = 1.0f / det;

		const float tx = (bax * cav - cax * bav) * invDet;
		const float ty = (bay * cav - cay * bav) * invDet;
		const float tz = (baz * cav - caz * bav) * invDet;

		const float bx = (cax * bau - bax * cau) * invDet;
		const float by = (cay * bau - bay * cau) * invDet;
		const float bz = (caz * bau - baz * cau) * invDet;

		for (uint32_t jj = 0; jj < 3; ++jj)
		{
			float* tanu = &tangents[indices[jj]*6];
			float* tanv = &tanu[3];
			tanu[0] += tx;
			tanu[1] += ty;
			tanu[2] += tz;

			tanv[0] += bx;
			tanv[1] += by;
			tanv[2] += bz;
		}
	}

	for (uint32_t ii = 0; ii < _numVertices; ++ii)
	{
		const float* tanu = &tangents[ii*6];
		const float* tanv = &tangents[ii*6 + 3];

		float normal[4];
		bgfx::vertexUnpack(normal, bgfx::Attrib::Normal, _decl, _vertices, ii);
		float ndt = bx::vec3Dot(normal, tanu);

		float nxt[3];
		bx::vec3Cross(nxt, normal, tanu);

		float tmp[3];
		tmp[0] = tanu[0] - normal[0] * ndt;
		tmp[1] = tanu[1] - normal[1] * ndt;
		tmp[2] = tanu[2] - normal[2] * ndt;

		float tangent[4];
		bx::vec3Norm(tangent, tmp);

		tangent[3] = bx::vec3Dot(nxt, tanv) < 0.0f ? -1.0f : 1.0f;
		bgfx::vertexPack(tangent, true, bgfx::Attrib::Tangent, _decl, _vertices, ii);
	}

	delete [] tangents;
}

struct Aabb
{
	float m_min[3];
	float m_max[3];
};

struct Obb
{
	float m_mtx[16];
};

struct Sphere
{
	float m_center[3];
	float m_radius;
};

struct Primitive
{
	uint32_t m_startIndex;
	uint32_t m_numIndices;
	uint32_t m_startVertex;
	uint32_t m_numVertices;

	Sphere m_sphere;
	Aabb m_aabb;
	Obb m_obb;
};

typedef stl::vector<Primitive> PrimitiveArray;

struct Group
{
	Group()
	{
		reset();
	}

	void reset()
	{
		m_vbh.idx = bgfx::invalidHandle;
		m_ibh.idx = bgfx::invalidHandle;
		m_prims.clear();
	}

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	Sphere m_sphere;
	Aabb m_aabb;
	Obb m_obb;
	PrimitiveArray m_prims;
};


struct Mesh
{
	void load(bx::ReaderSeekerI* _reader)
	{
#define BGFX_CHUNK_MAGIC_VB  BX_MAKEFOURCC('V', 'B', ' ', 0x1)
#define BGFX_CHUNK_MAGIC_IB  BX_MAKEFOURCC('I', 'B', ' ', 0x0)
#define BGFX_CHUNK_MAGIC_IBC BX_MAKEFOURCC('I', 'B', 'C', 0x0)
#define BGFX_CHUNK_MAGIC_PRI BX_MAKEFOURCC('P', 'R', 'I', 0x0)

		using namespace bx;
		using namespace bgfx;

		Group group;

		bx::AllocatorI* allocator = entry::getAllocator();

		uint32_t chunk;
		while (4 == bx::read(_reader, chunk) )
		{
			switch (chunk)
			{
			case BGFX_CHUNK_MAGIC_VB:
				{
					read(_reader, group.m_sphere);
					read(_reader, group.m_aabb);
					read(_reader, group.m_obb);

					read(_reader, m_decl);

					uint16_t stride = m_decl.getStride();

					uint16_t numVertices;
					read(_reader, numVertices);
					const bgfx::Memory* mem = bgfx::alloc(numVertices*stride);
					read(_reader, mem->data, mem->size);

					group.m_vbh = bgfx::createVertexBuffer(mem, m_decl);
				}
				break;

			case BGFX_CHUNK_MAGIC_IB:
				{
					uint32_t numIndices;
					read(_reader, numIndices);
					const bgfx::Memory* mem = bgfx::alloc(numIndices*2);
					read(_reader, mem->data, mem->size);
					group.m_ibh = bgfx::createIndexBuffer(mem);
				}
				break;

			case BGFX_CHUNK_MAGIC_IBC:
				{
					uint32_t numIndices;
					bx::read(_reader, numIndices);

					const bgfx::Memory* mem = bgfx::alloc(numIndices*2);

					uint32_t compressedSize;
					bx::read(_reader, compressedSize);

					void* compressedIndices = BX_ALLOC(allocator, compressedSize);

					bx::read(_reader, compressedIndices, compressedSize);

					ReadBitstream rbs( (const uint8_t*)compressedIndices, compressedSize);
					DecompressIndexBuffer( (uint16_t*)mem->data, numIndices / 3, rbs);

					BX_FREE(allocator, compressedIndices);

					group.m_ibh = bgfx::createIndexBuffer(mem);
				}
				break;

			case BGFX_CHUNK_MAGIC_PRI:
				{
					uint16_t len;
					read(_reader, len);

					stl::string material;
					material.resize(len);
					read(_reader, const_cast<char*>(material.c_str() ), len);

					uint16_t num;
					read(_reader, num);

					for (uint32_t ii = 0; ii < num; ++ii)
					{
						read(_reader, len);

						stl::string name;
						name.resize(len);
						read(_reader, const_cast<char*>(name.c_str() ), len);

						Primitive prim;
						read(_reader, prim.m_startIndex);
						read(_reader, prim.m_numIndices);
						read(_reader, prim.m_startVertex);
						read(_reader, prim.m_numVertices);
						read(_reader, prim.m_sphere);
						read(_reader, prim.m_aabb);
						read(_reader, prim.m_obb);

						group.m_prims.push_back(prim);
					}

					m_groups.push_back(group);
					group.reset();
				}
				break;

			default:
				DBG("%08x at %d", chunk, bx::skip(_reader, 0) );
				break;
			}
		}
	}

	void unload()
	{
		for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
		{
			const Group& group = *it;
			bgfx::destroyVertexBuffer(group.m_vbh);

			if (bgfx::isValid(group.m_ibh) )
			{
				bgfx::destroyIndexBuffer(group.m_ibh);
			}
		}
		m_groups.clear();
	}

	void submit(uint8_t _id, bgfx::ProgramHandle _program, const float* _mtx, uint64_t _state) const
	{
		if (BGFX_STATE_MASK == _state)
		{
			_state = 0
				| BGFX_STATE_RGB_WRITE
				| BGFX_STATE_ALPHA_WRITE
				| BGFX_STATE_DEPTH_WRITE
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_MSAA
				;
		}

		uint32_t cached = bgfx::setTransform(_mtx);

		for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
		{
			const Group& group = *it;

			bgfx::setTransform(cached);
			bgfx::setIndexBuffer(group.m_ibh);
			bgfx::setVertexBuffer(group.m_vbh);
			bgfx::setState(_state);
			bgfx::submit(_id, _program);
		}
	}

	void submit(const RenderState* _state, uint8_t _numPasses, const float* _mtx, uint16_t _numMatrices) const
	{
		uint32_t cached = bgfx::setTransform(_mtx, _numMatrices);

		for (uint32_t pass = 0; pass < _numPasses; ++pass)
		{
			const RenderState& state = _state[pass];

			for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
			{
				const Group& group = *it;

				bgfx::setTransform(cached, _numMatrices);
				for (uint8_t tex = 0; tex < state.m_numTextures; ++tex)
				{
					const RenderState::Texture& texture = state.m_textures[tex];
					bgfx::setTexture(texture.m_stage
							, texture.m_sampler
							, texture.m_texture
							, texture.m_flags
							);
				}
				bgfx::setIndexBuffer(group.m_ibh);
				bgfx::setVertexBuffer(group.m_vbh);
				bgfx::setState(state.m_state);
				bgfx::submit(state.m_viewId, state.m_program);
			}
		}
	}

	bgfx::VertexDecl m_decl;
	typedef stl::vector<Group> GroupArray;
	GroupArray m_groups;
};

Mesh* meshLoad(bx::ReaderSeekerI* _reader)
{
	Mesh* mesh = new Mesh;
	mesh->load(_reader);
	return mesh;
}

Mesh* meshLoad(const char* _filePath)
{
	bx::FileReaderI* reader = entry::getFileReader();
	bx::open(reader, _filePath);
	Mesh* mesh = meshLoad(reader);
	bx::close(reader);
	return mesh;
}

void meshUnload(Mesh* _mesh)
{
	_mesh->unload();
	delete _mesh;
}

void meshSubmit(const Mesh* _mesh, uint8_t _id, bgfx::ProgramHandle _program, const float* _mtx, uint64_t _state)
{
	_mesh->submit(_id, _program, _mtx, _state);
}

void meshSubmit(const Mesh* _mesh, const RenderState* _state, uint8_t _numPasses, const float* _mtx, uint16_t _numMatrices)
{
	_mesh->submit(_state, _numPasses, _mtx, _numMatrices);
}

uint32_t packUint32(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w)
{
	union
	{
		uint32_t ui32;
		uint8_t arr[4];
	} un;

	un.arr[0] = _x;
	un.arr[1] = _y;
	un.arr[2] = _z;
	un.arr[3] = _w;

	return un.ui32;
}

uint32_t packF4u(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const uint8_t xx = uint8_t(_x*127.0f + 128.0f);
	const uint8_t yy = uint8_t(_y*127.0f + 128.0f);
	const uint8_t zz = uint8_t(_z*127.0f + 128.0f);
	const uint8_t ww = uint8_t(_w*127.0f + 128.0f);
	return packUint32(xx, yy, zz, ww);
}

struct PosNormalColorVertex {
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_normal;
	uint32_t m_rgba;

	static void init() {
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true, true)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosNormalColorVertex::ms_decl;

struct PosNormalTexcoordVertex
{
	float    m_x;
	float    m_y;
	float    m_z;
	uint32_t m_normal;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosNormalTexcoordVertex::ms_decl;

static const PosNormalTexcoordVertex s_cubeVertices[] =
{
	{ -1.0f,  1.0f,  1.0f, packF4u( 0.0f,  1.0f,  0.0f)},
	{  1.0f,  1.0f,  1.0f, packF4u( 0.0f,  1.0f,  0.0f)},
	{ -1.0f,  1.0f, -1.0f, packF4u( 0.0f,  1.0f,  0.0f)},
	{  1.0f,  1.0f, -1.0f, packF4u( 0.0f,  1.0f,  0.0f)},
	{ -1.0f, -1.0f,  1.0f, packF4u( 0.0f, -1.0f,  0.0f)},
	{  1.0f, -1.0f,  1.0f, packF4u( 0.0f, -1.0f,  0.0f)},
	{ -1.0f, -1.0f, -1.0f, packF4u( 0.0f, -1.0f,  0.0f)},
	{  1.0f, -1.0f, -1.0f, packF4u( 0.0f, -1.0f,  0.0f)},
	{  1.0f, -1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f)},
	{  1.0f,  1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f)},
	{ -1.0f, -1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f)},
	{ -1.0f,  1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f)},
	{  1.0f, -1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f)},
	{  1.0f,  1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f)},
	{ -1.0f, -1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f)},
	{ -1.0f,  1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f)},
	{  1.0f,  1.0f, -1.0f, packF4u( 1.0f,  0.0f,  0.0f)},
	{  1.0f,  1.0f,  1.0f, packF4u( 1.0f,  0.0f,  0.0f)},
	{  1.0f, -1.0f, -1.0f, packF4u( 1.0f,  0.0f,  0.0f)},
	{  1.0f, -1.0f,  1.0f, packF4u( 1.0f,  0.0f,  0.0f)},
	{ -1.0f,  1.0f, -1.0f, packF4u(-1.0f,  0.0f,  0.0f)},
	{ -1.0f,  1.0f,  1.0f, packF4u(-1.0f,  0.0f,  0.0f)},
	{ -1.0f, -1.0f, -1.0f, packF4u(-1.0f,  0.0f,  0.0f)},
	{ -1.0f, -1.0f,  1.0f, packF4u(-1.0f,  0.0f,  0.0f)},
};

static const uint16_t s_cubeIndices[] =
{
	0,  1,  2,
	1,  3,  2,
	4,  6,  5,
	5,  6,  7,

	8,  9, 10,
	9, 11, 10,
	12, 14, 13,
	13, 14, 15,

	16, 17, 18,
	17, 19, 18,
	20, 22, 21,
	21, 22, 23,
};

void mesh_load(Mesh* mesh, const void* _vertices, uint32_t _numVertices, const bgfx::VertexDecl _decl
	, const uint16_t* _indices, uint32_t _numIndices)
{
	Group group;
	const bgfx::Memory* mem;
	uint32_t size;

	size = _numVertices*_decl.getStride();
	mem = bgfx::makeRef(_vertices, size);
	group.m_vbh = bgfx::createVertexBuffer(mem, _decl);

	size = _numIndices*2;
	mem = bgfx::makeRef(_indices, size);
	group.m_ibh = bgfx::createIndexBuffer(mem);

	//TODO:
	// group.m_sphere = ...
	// group.m_aabb = ...
	// group.m_obb = ...
	// group.m_prims = ...

	mesh->m_groups.push_back(group);
}

// Mesh *createMeshFromVBO (const MeshVBO& mesh_buffer) {
// 	PosNormalTexcoordVertex::init();
// 	Mesh* result = new Mesh();
// 
// 	const std::vector<Vector4f>& vertices = mesh_buffer.vertices;
// 	const std::vector<Vector3f>& normals = mesh_buffer.normals;
// 	const std::vector<Vector4f>& colors = mesh_buffer.colors;
// 
// 	bool have_normals = mesh_buffer.normals.size() > 0;
// 	bool have_colors = mesh_buffer.colors.size() > 0;
// 
// 	PosNormalColorVertex::init();
// 
// 	uint16_t stride = PosNormalColorVertex::ms_decl.getStride();
// 	const bgfx::Memory* vb_mem = bgfx::alloc (vertices.size() * stride);
// 	PosNormalColorVertex* mesh_vb = (PosNormalColorVertex*) vb_mem;
// 
// 	const bgfx::Memory* ib_mem = bgfx::alloc (sizeof(uint16_t) * vertices.size());
// 	uint16_t* mesh_ib = (uint16_t*) ib_mem;
// 
// 	for (unsigned int i = 0; i < mesh_buffer.vertices.size(); i++) {
// 		mesh_vb[i].m_x = vertices[i][0];
// 		mesh_vb[i].m_y = vertices[i][1];
// 		mesh_vb[i].m_z = vertices[i][2];
// 
// 		if (have_normals) {
// 			mesh_vb[i].m_normal = packF4u (-normals[i][0], -normals[i][1], -normals[i][2]);
// 		} else {
// 			mesh_vb[i].m_normal = 0;
// 		}
// 
// 		if (have_colors) {
// 			mesh_vb[i].m_rgba = packF4u (colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
// 		} else {
// 			mesh_vb[i].m_rgba = packF4u (1.f, 1.f, 1.f, 1.f);
// 		}
// 
// 		mesh_ib[i] = i;
// 	}
// 
// 	mesh_load(result, mesh_vb, vertices.size(), PosNormalColorVertex::ms_decl, mesh_ib, vertices.size());
// 
// 	return result;
// }

}


