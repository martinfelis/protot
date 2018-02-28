#include <string.h> // strlen
#include <locale.h>

#include <iostream>
#include <fstream>

#include "RenderModule.h"
#include "RenderUtils.h"
#include "Globals.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb/stb_image.h"

using namespace SimpleMath;

//
// RenderProgram
//

RenderProgram::~RenderProgram() {
	if (mProgramId != -1)
		glDeleteProgram(mProgramId);
}

bool RenderProgram::Load() {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(mVertexShaderFilename.c_str(), std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		gLog("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !", mVertexShaderFilename.c_str());
		getchar();
		return false;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(mFragmentShaderFilename.c_str(), std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	gLog("Compiling shader : %s", mVertexShaderFilename.c_str());
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		gLog("%s", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	gLog("Compiling shader : %s", mFragmentShaderFilename.c_str());
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		gLog("%s", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	gLog("Linking program");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		gLog("%s", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	mProgramId = ProgramID;
	return true;
}

GLuint RenderProgram::GetUniformLocation(const std::string& name) {
	if (mProgramId == -1) {
		gLog("Cannot get uniform '%s' for program '%s' and '%s': shader not valid.", 
				name.c_str(),
				mVertexShaderFilename.c_str(),
				mFragmentShaderFilename.c_str()
				);
		assert(mProgramId != -1);
	}
	GLuint result = glGetUniformLocation(mProgramId, name.c_str());
	if (result == -1) {
		gLog ("Error loading uniform '%s' from shaders '%s' and '%s': uniform not found.",
				name.c_str(),
				mVertexShaderFilename.c_str(), 
				mFragmentShaderFilename.c_str()
				);
		assert(false);
	} else {
		gLog ("Uniform '%s': %d", name.c_str(), result);
	}

	return result;
}

//
// RenderTarget
//
RenderTarget::RenderTarget(int width, int height, int flags) {
	mFlags = flags;

	Cleanup();
	Resize(width, height);
}

RenderTarget::~RenderTarget() {
	Cleanup();
}

void RenderTarget::Cleanup() {
	if (mFrameBufferId != -1) {
		glDeleteFramebuffers(1, &mFrameBufferId);
		mFrameBufferId = -1;
	}

	if (mColorTexture != -1) {
		glDeleteTextures(1, &mColorTexture);
		mColorTexture = -1;
	}

	if (mDepthTexture!= -1) {
		glDeleteTextures(1, &mDepthTexture);
		mDepthTexture = -1;
	}

	if (mDepthBuffer != -1) {
		glDeleteRenderbuffers(1, &mDepthBuffer);
		mDepthBuffer = -1;
	}

	if (mLinearizedDepthTexture != -1) {
		glDeleteTextures(1, &mLinearizedDepthTexture);
		mLinearizedDepthTexture = -1;
	}
}

void RenderTarget::Resize(int width, int height) {
	if (width == mWidth || height == mHeight)
		return;

	Cleanup();

	mWidth = width;
	mHeight = height;

	glGenFramebuffers(1, &mFrameBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);

	if (mFlags & EnableColor) {
		glGenTextures(1, &mColorTexture);
		glBindTexture(GL_TEXTURE_2D, mColorTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTexture, 0);
	}

	if (mFlags & EnableDepthTexture) {
		assert((mFlags & EnableDepth) == false);

		glGenTextures(1, &mDepthTexture);
		glBindTexture(GL_TEXTURE_2D, mDepthTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mWidth, mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthTexture, 0);

		if (mFlags & EnableLinearizedDepthTexture) {
			glGenTextures(1, &mLinearizedDepthTexture);
			glBindTexture(GL_TEXTURE_2D, mLinearizedDepthTexture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	} else if (mFlags & EnableDepth) {
		assert((mFlags & EnableDepthTexture) == false);
		glGenRenderbuffers(1, &mDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
	}
}

void RenderTarget::RenderToLinearizedDepth(bool render_to_depth) {
	if (render_to_depth) {
		assert(mFlags & EnableLinearizedDepthTexture);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mLinearizedDepthTexture, 0);
	} else {
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTexture, 0);
	}
}

//
// Texture
//

Texture::~Texture() {
	if (mTextureId != -1) {
		glDeleteTextures(1, &mTextureId);
		mTextureId = -1;
	}
}

void Texture::MakeGrid(const int& size, const Vector3f &c1, const Vector3f &c2) {
	mWidth = size;
	mHeight = size;

	unsigned char buffer[size * size * 3];
	int size_half = size / 2;

	for (int i = 0; i < size_half; ++i) {
		for (int j = 0; j < size_half; ++j) {
			buffer[(i * size * 3) + (j * 3) + 0] = static_cast<unsigned char>(c1[0] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 1] = static_cast<unsigned char>(c1[1] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 2] = static_cast<unsigned char>(c1[2] * 255.0f);
		}
	}

	for (int i = size_half; i < size; ++i) {
		for (int j = 0; j < size_half; ++j) {
			buffer[(i * size * 3) + (j * 3) + 0] = static_cast<unsigned char>(c2[0] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 1] = static_cast<unsigned char>(c2[1] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 2] = static_cast<unsigned char>(c2[2] * 255.0f);
		}
	}

	for (int i = size_half; i < size; ++i) {
		for (int j = size_half; j < size; ++j) {
			buffer[(i * size * 3) + (j * 3) + 0] = static_cast<unsigned char>(c1[0] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 1] = static_cast<unsigned char>(c1[1] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 2] = static_cast<unsigned char>(c1[2] * 255.0f);
		}
	}

	for (int i = 0; i < size_half; ++i) {
		for (int j = size_half; j < size; ++j) {
			buffer[(i * size * 3) + (j * 3) + 0] = static_cast<unsigned char>(c2[0] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 1] = static_cast<unsigned char>(c2[1] * 255.0f);
			buffer[(i * size * 3) + (j * 3) + 2] = static_cast<unsigned char>(c2[2] * 255.0f);
		}
	}

	glGenTextures(1, &mTextureId);
	glBindTexture(GL_TEXTURE_2D, mTextureId);

	glTexImage2D(GL_TEXTURE_2D, 
			0,								// level
			GL_RGB,						// internal format
			size,							// width
			size,							// height
			0,								// border (must be 0)
			GL_RGB,						// format of pixel data
			GL_UNSIGNED_BYTE,	// type of pixel data
			buffer						// pixel data
			);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

bool Texture::Load(const char* filename, int num_components) {
//	unsigned char* rgb = stbi_load(filename, &mWidth, &mHeight, num_components);
	assert(false);
}

VertexArray::~VertexArray() {
	if (mVertexArrayId != -1) {
		Cleanup();
	}
}

void VertexArray::Initialize(const int& size, GLenum usage) {
	mNumVertices = size;
	mNumUsedVertices = 0;

	glGenVertexArrays (1, &mVertexArrayId);
	glBindVertexArray(mVertexArrayId);

	glGenBuffers(1, &mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * mNumVertices, NULL, usage);
}

void VertexArray::Cleanup() {
	glDeleteBuffers(1, &mVertexBuffer);
	mVertexBuffer = -1;
	glDeleteVertexArrays(1, &mVertexArrayId);
	mVertexArrayId = -1;

	mNumVertices = -1;
	mNumUsedVertices = -1;
}

GLuint VertexArray::AllocateMesh(const int& size) {
	if (mNumUsedVertices + size > mNumVertices) {
		gLog("Cannot allocate mesh in VertexArray: not enough vertices available");
		assert(false);
		return -1;
	}

	GLuint offset = mNumUsedVertices;
	mNumUsedVertices += size;
	return offset;
}

void VertexArray::Bind() {
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

	// Attribute 0: coords
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
			0,
			4,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)0
			);
	// Attribute 1: color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
			1,
			4,
			GL_UNSIGNED_BYTE,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 9)
			);
	// Attribute 2: normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
			2,
			3,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 4)
			);
	// Attribute 3: texture coordinates
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(
			3,
			2,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 7)
			);
}

bool VertexArray::IsBound() {
	GLint bound_vertex_buffer = -1;
	glGetIntegerv (GL_ARRAY_BUFFER_BINDING, &bound_vertex_buffer);
	return bound_vertex_buffer == mVertexBuffer;
}

void VertexArrayMesh::Initialize(VertexArray &array, const int& size) {
	mVertexArray = &array;
	mIndexOffset = mVertexArray->AllocateMesh(size);
	mOffsetPtr = (void*) (sizeof(VertexArray::VertexData) * mIndexOffset);
}

void VertexArrayMesh::SetData(
		const VertexArray::VertexData* data,
		const int& count
		) {
	// upload the data
	mVertexCount = count;
	glBindBuffer(GL_ARRAY_BUFFER, mVertexArray->mVertexBuffer);

	glBufferSubData(
			GL_ARRAY_BUFFER,
			(GLintptr) mOffsetPtr,
			sizeof(VertexArray::VertexData) * count,
			data
			);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexArrayMesh::SetData(
		const std::vector<Vector4f> &coords,
		const std::vector<Vector3f> &normals,
		const std::vector<Vector2f> &uvs,
		const std::vector<Vector4f> &colors,
		const std::vector<GLuint> &indices
		) {
	assert(mOffsetPtr != (void*) -1);

	int vertex_count = coords.size();
	assert(vertex_count > 0);

	bool have_normals = normals.size() > 0;
	bool have_uvs = uvs.size() > 0;
	bool have_colors = colors.size() > 0;
	
	assert(!have_normals || (normals.size() == vertex_count));
	assert(!have_uvs || (uvs.size() == vertex_count));
	assert(!have_colors || (colors.size() == vertex_count));

	std::vector<VertexArray::VertexData> vertex_data(vertex_count, VertexArray::VertexData());

	for (int i = 0, n = vertex_count; i < n; ++i) {
		memcpy (vertex_data[i].mCoords, coords[i].data(), sizeof(float) * 4);

		if (have_normals) {
			memcpy (vertex_data[i].mNormals, normals[i].data(), sizeof(float) * 3);
		} else {
			memset (vertex_data[i].mNormals, 0, sizeof(float) * 3);
		}

		if (have_uvs) {
			memcpy (vertex_data[i].mTexCoords, uvs[i].data(), sizeof(float) * 2);
		} else {
			memset (vertex_data[i].mTexCoords, 0, sizeof(float) * 2);
		}

		if (have_colors) {
			memcpy (vertex_data[i].mColor, colors[i].data(), sizeof(float) * 4);
		} else {
			memset (vertex_data[i].mColor, 0, sizeof(float) * 4);
		}
	}

	SetData(
			vertex_data.data(),
			vertex_count
			);
}

void VertexArrayMesh::Draw(GLenum mode) {
	assert(mVertexArray->IsBound());
	glDrawArrays(mode, mIndexOffset, mVertexCount); 
}
