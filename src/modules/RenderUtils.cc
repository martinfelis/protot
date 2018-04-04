#include <string.h> // strlen
#include <locale.h>

#include <iostream>
#include <fstream>

#include "RenderModule.h"
#include "RenderUtils.h"
#include "Globals.h"
#include "FileModificationObserver.h"

#include "imgui/imgui.h"
#include "imgui_dock.h"

using namespace SimpleMath;
using namespace SimpleMath::GL;

typedef tinygltf::TinyGLTF GLTFLoader;

static GLTFLoader gLoader;

//
// Camera
//
void Camera::UpdateMatrices() {
	mViewMatrix = LookAt(mEye, mPoi, mUp);

	if (mIsOrthographic) {
		float width = mWidth * 0.5f * (mFar - mNear * 0.5f) * 0.001f;
		float height = width * mHeight / mWidth;

		mProjectionMatrix = Ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, mNear, mFar);
	} else {
		mProjectionMatrix = Perspective(mFov, mWidth / mHeight, mNear, mFar);
	}
}

void Camera::DrawGui() {
	ImGui::Text("Width %3.4f, Height %3.4f", mWidth, mHeight);

	ImGui::InputFloat3("Eye", mEye.data(), -10.0f, 10.0f);
	ImGui::SliderFloat3("EyeS", mEye.data(), -10.0f, 10.0f);

	ImGui::InputFloat3("Poi", mPoi.data(), -10.0f, 10.0f);
	ImGui::InputFloat3("Up", mUp.data(), -10.0f, 10.0f);
	ImGui::Checkbox("Orthographic", &mIsOrthographic);
	ImGui::SliderFloat("Fov", &mFov, 5, 160);
	ImGui::SliderFloat("Near", &mNear, -10, 10);
	ImGui::SliderFloat("Far", &mFar, -20, 50);
	if (ImGui::Button("Reset")) {
		*this = Camera();
	}
}


//
// RenderProgram
//

RenderProgram::~RenderProgram() {
	if (mProgramId != -1)
		glDeleteProgram(mProgramId);
}


GLuint RenderProgram::CompileVertexShader() {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);

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
		return -1;
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

		return -1;
	}

	return VertexShaderID;
}

GLuint RenderProgram::CompileFragmentShader() {
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

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
		
		return -1;
	}

	return FragmentShaderID;
}

GLuint RenderProgram::LinkProgram(GLuint vertex_shader, GLuint fragment_shader) {
	// Link the program
	gLog("Linking program");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, vertex_shader);
	glAttachShader(ProgramID, fragment_shader);

	// Bind attribute locations
	glBindAttribLocation(ProgramID, 0, "inCoord");
	glBindAttribLocation(ProgramID, 1, "inNormal");
	glBindAttribLocation(ProgramID, 2, "inUV");
	glBindAttribLocation(ProgramID, 3, "inColor");

	glBindFragDataLocation(ProgramID, 0, "outColor");
	glBindFragDataLocation(ProgramID, 1, "outPosition");
	glBindFragDataLocation(ProgramID, 2, "outNormal");

	glLinkProgram(ProgramID);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		gLog("%s", &ProgramErrorMessage[0]);
	}

	return ProgramID;
}

bool RenderProgram::Load() {
	GLuint vertex_shader_id = CompileVertexShader();
	if (vertex_shader_id == -1) {
		return false;
	}

	GLuint fragment_shader_id = CompileFragmentShader();
	if (fragment_shader_id == -1) {
		glDeleteShader(vertex_shader_id);
		return false;
	}

	mProgramId = LinkProgram(vertex_shader_id, fragment_shader_id);
	if (mProgramId == -1) {
		glDeleteShader(vertex_shader_id);
		glDeleteShader(fragment_shader_id);

		return false;
	}

	glDetachShader(mProgramId, vertex_shader_id);
	glDetachShader(mProgramId, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

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

void RenderProgram::RegisterFileModification() {
	gFileModificationObserver->AddListener(mVertexShaderFilename, this);	
	gFileModificationObserver->AddListener(mFragmentShaderFilename, this);	
}

bool RenderProgram::OnFileChanged(const std::string& filename) {
	gLog("Renderprogram reload as file %s changed", filename.c_str());

	GLuint vertex_shader_id = CompileVertexShader();
	if (vertex_shader_id == -1) {
		gLog ("Reload failed: error when compiling vertex shader");
		return true;
	}

	GLuint fragment_shader_id = CompileFragmentShader();
	if (fragment_shader_id == -1) {
		glDeleteShader(vertex_shader_id);
		gLog ("Reload failed: error when compiling fragment shader");
		return false;
	}

	mProgramId = LinkProgram(vertex_shader_id, fragment_shader_id);
	if (mProgramId == -1) {
		glDeleteShader(vertex_shader_id);
		glDeleteShader(fragment_shader_id);

		gLog ("Reload failed: error when linking the program");

		return false;
	}

	glDetachShader(mProgramId, vertex_shader_id);
	glDetachShader(mProgramId, fragment_shader_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	gLog ("Reload successful");

	return true;
}

//
// RenderTarget
//
RenderTarget::~RenderTarget() {
	Cleanup();
}

void RenderTarget::Initialize(int width, int height, int flags) {
	Cleanup();

	mFlags = flags;

	Resize(width, height, mFlags);
}

void RenderTarget::Bind() {
	assert(glIsFramebuffer(mFrameBufferId));
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);

	GLenum buffers[8];
	int num_buffers = 0;

	if (mFlags & EnableColor) {
		buffers[num_buffers++] = GL_COLOR_ATTACHMENT0;
	}

	if (mFlags & EnablePositionTexture ) {
		buffers[num_buffers++] = GL_COLOR_ATTACHMENT2;
	}

	if (mFlags & EnableNormalTexture) {
		buffers[num_buffers++] = GL_COLOR_ATTACHMENT3;
	}

	glDrawBuffers(num_buffers, buffers);
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

	if (mPositionTexture != -1) {
		glDeleteTextures(1, &mPositionTexture);
		mPositionTexture = -1;
	}

	if (mNormalTexture != -1) {
		glDeleteTextures(1, &mNormalTexture);
		mNormalTexture = -1;
	}

	mWidth = -1;
	mHeight = -1;
	mFlags = 0;
}

void RenderTarget::Resize(int width, int height, int flags) {
	if (width == mWidth && height == mHeight && flags == mFlags)
		return;

	Cleanup();

	mFlags = flags;

	gLog("Resizing RenderTarget to %d,%d flags: %d", width, height, flags);

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

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture, 0);
	}

	if (mFlags & EnableDepthTexture) {
		assert((mFlags & EnableDepth) == false);

		glGenTextures(1, &mDepthTexture);
		glBindTexture(GL_TEXTURE_2D, mDepthTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mWidth, mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

		// Set parameters so that we can set a shadow2DSampler
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture, 0);

		if (mFlags & EnableLinearizedDepthTexture) {
			glGenTextures(1, &mLinearizedDepthTexture);
			glBindTexture(GL_TEXTURE_2D, mLinearizedDepthTexture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mLinearizedDepthTexture, 0);
		}
	} else if (mFlags & EnableDepth) {
		assert((mFlags & EnableDepthTexture) == false);
		glGenRenderbuffers(1, &mDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
	}

	if (mFlags & EnablePositionTexture) {
		glGenTextures(1, &mPositionTexture);
		glBindTexture(GL_TEXTURE_2D, mPositionTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mPositionTexture, 0);
	}

	if (mFlags & EnableNormalTexture) {
		glGenTextures(1, &mNormalTexture);
		glBindTexture(GL_TEXTURE_2D, mNormalTexture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mNormalTexture, 0);
	}

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result != GL_FRAMEBUFFER_COMPLETE) {
		switch (result) {
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: gLog("Error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
																								 break;
			case GL_FRAMEBUFFER_UNDEFINED: gLog("Error: GL_FRAMEBUFFER_UNDEFINED");
																								 break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: gLog("Error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
																												 break;
			default: gLog("Error when creating Framebuffer: %d", result);
		}
	}

	assert(result == GL_FRAMEBUFFER_COMPLETE);
}

void RenderTarget::RenderToLinearizedDepth(const float& near, const float& far, bool is_orthographic) {
	assert(mFlags & EnableLinearizedDepthTexture);
	assert(mLinearizedDepthTexture != -1);
	assert(mVertexArray != nullptr);
	assert(mQuadMesh != nullptr);

	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);
	GLenum draw_attachment_1[] = { GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(1, draw_attachment_1);

	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mDepthTexture);

	// render depth texture
	glUseProgram(mLinearizeDepthProgram.mProgramId);
	mLinearizeDepthProgram.SetFloat("uNear", near);
	mLinearizeDepthProgram.SetFloat("uFar", far);
	mLinearizeDepthProgram.SetFloat("uIsOrthographic", is_orthographic ? 1.0f : 0.0f);
	mLinearizeDepthProgram.SetInt("uDepthTexture", 0);

	mVertexArray->Bind();
	mQuadMesh->Draw(GL_TRIANGLES);

	if (mFlags & EnableColor) {
		GLenum draw_attachment_0[] = { GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(1, draw_attachment_0);
		glEnable(GL_DEPTH_TEST);
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
	return false;
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
			VertexAttributePosition,
			4,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)0
			);
	// Attribute 1: normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
			VertexAttributeNormal,
			3,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 4)
			);
	// Attribute 2: texture coordinates
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
			VertexAttributeTexCoord0,
			2,
			GL_FLOAT,
			GL_FALSE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 7)
			);
	// Attribute 3: color
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(
			VertexAttributeColor,
			4,
			GL_UNSIGNED_BYTE,
			GL_TRUE,
			(sizeof(VertexData)),
			(void*)(sizeof(float) * 9)
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
			for (int j = 0; j < 4; ++j) {
				vertex_data[i].mColor[j] = GLubyte(colors[i][j] * 255.0f);
			}
		} else {
			memset (vertex_data[i].mColor, 0, sizeof(float) * 4);
		}
	}

	SetData(
			vertex_data.data(),
			vertex_count
			);
}

void VertexArrayMesh::SetIndexData(const GLuint* indices, const int& count) {
	assert(mIndexBuffer == -1);

	// copy the indices and increase the indices by mIndexOffset
	GLuint temp_buffer[count];
	memcpy (temp_buffer, indices, sizeof(GLuint) * count);
	for (int i = 0; i < count; ++i) {
		temp_buffer[i] += mIndexOffset;
	}

	mIndexCount = count;

	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLuint), temp_buffer, GL_STATIC_DRAW);
}

void VertexArrayMesh::Draw(GLenum mode) {
	assert(mVertexArray->IsBound());

	if (mIndexBuffer == -1) {
		glDrawArrays(mode, mIndexOffset, mVertexCount); 
	} else {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
		glDrawElements(mode, mIndexCount, GL_UNSIGNED_INT, (void*) 0);
	}

}

//
// AssetFile
//
bool AssetFile::Load(const char* filename) {
	mFilename = filename;
	std::string err;
	bool result = gLoader.LoadASCIIFromFile(&mGLTFModel, &err, mFilename.c_str());
	if (!err.empty()) {
		gLog("Error loading model '%s': %s", mFilename.c_str(), err.c_str());
	} else {
		gLog("Successfully loaded model '%s'", mFilename.c_str());
	}
	
	LoadBuffers();

	return result;
}

void AssetFile::DrawNodeGui(const tinygltf::Node& node) {
	for (int i = 0, n = node.children.size(); i < n; ++i) {
		const int child_node_id = node.children[i];
		const tinygltf::Node& child_node = mGLTFModel.nodes[child_node_id];
		ImGui::PushID("childnode");
		ImGui::PushID("i");
		if (ImGui::TreeNode((void*)(intptr_t)child_node_id, "[%d] %s", child_node_id, child_node.name.c_str())) {
			DrawNodeGui(child_node);
			ImGui::TreePop();
		}
		ImGui::PopID();
		ImGui::PopID();
	}
}

void AssetFile::LoadBuffers() {
	for (int i = 0, n = mGLTFModel.bufferViews.size(); i < n; ++i) {
		const tinygltf::BufferView &buffer_view = mGLTFModel.bufferViews[i];
		if (buffer_view.target == 0) {
			gLog("Warning: buffer_view target at index %d is 0", i);
			continue;
		}

		const tinygltf::Buffer &buffer = mGLTFModel.buffers[buffer_view.buffer];
		GLuint buffer_id;
		glGenBuffers(1, &buffer_id);
		glBindBuffer(buffer_view.target, buffer_id);
		if (buffer_view.name.size() > 0) {
			gLog("Loading Buffer '%s': size %d offset %d", buffer_view.name.c_str(), buffer.data.size(), buffer_view.byteOffset);
		} else {
			gLog("Loading Buffer: size %d offset %d", buffer_view.name.c_str(), buffer.data.size(), buffer_view.byteOffset);
		}
		glBufferData(buffer_view.target, buffer_view.byteLength,
				&buffer.data.at(0) + buffer_view.byteOffset, GL_STATIC_DRAW);
		glBindBuffer(buffer_view.target, 0);
		mBuffers.push_back(buffer_id);
	}
}

VertexAttributeType AssetFile::GetVertexAttributeType(const std::string &attribute_string) const {
	VertexAttributeType attribute_type;
	if (attribute_string.compare("POSITION") == 0) {
		attribute_type = VertexAttributePosition;
	} else if (attribute_string.compare("NORMAL") == 0) {
		attribute_type = VertexAttributeNormal;
	} else if (attribute_string.compare("TEXCOORD_0") == 0) {
		attribute_type = VertexAttributeTexCoord0;
	} else {
		attribute_type = VertexAttributeTypeCount;
	}

	return attribute_type;
}

void AssetFile::DrawMesh(const tinygltf::Mesh &mesh, const Matrix44f& matrix) const {
	for (int i = 0, n = mesh.primitives.size(); i < n; ++i) {
		const tinygltf::Primitive& primitive = mesh.primitives[i];

		if (primitive.indices < 0) {
			return;
		}

		std::map<std::string, int>::const_iterator it(primitive.attributes.begin());
		std::map<std::string, int>::const_iterator it_end(primitive.attributes.end());

		for (; it != it_end; it++) {
			assert(it->second >= 0);
			const tinygltf::Accessor& accessor = mGLTFModel.accessors[it->second];
			glBindBuffer(GL_ARRAY_BUFFER, mBuffers[accessor.bufferView]);

			int size = 1;
			switch(accessor.type) {
				case TINYGLTF_TYPE_SCALAR:	size = 1; break;
				case TINYGLTF_TYPE_VEC2: 		size = 2; break;
				case TINYGLTF_TYPE_VEC3: 		size = 3; break;
				case TINYGLTF_TYPE_VEC4: 		size = 4; break;
				default: assert(0); break;
			}

			VertexAttributeType attribute_type = GetVertexAttributeType(it->first);

			if (attribute_type != VertexAttributeTypeCount) {
				int byte_stride = accessor.ByteStride(mGLTFModel.bufferViews[accessor.bufferView]);
				assert(byte_stride != -1);

				glVertexAttribPointer(
						attribute_type,
						size,
						accessor.componentType,
						accessor.normalized ? GL_TRUE : GL_FALSE,
						byte_stride,
						(char *)NULL + accessor.byteOffset);
				glEnableVertexAttribArray(attribute_type);
			}
		}

		const tinygltf::Accessor& index_accessor = mGLTFModel.accessors[primitive.indices];
		glBindBuffer(
				GL_ELEMENT_ARRAY_BUFFER,
				mBuffers[index_accessor.bufferView]
				);

		int mode = -1;
		switch (primitive.mode) {
			case TINYGLTF_MODE_TRIANGLES : mode = GL_TRIANGLES; break;
			case TINYGLTF_MODE_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
			case TINYGLTF_MODE_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
			case TINYGLTF_MODE_POINTS: mode = GL_POINTS; break;
			case TINYGLTF_MODE_LINE: mode = GL_LINES; break;
			case TINYGLTF_MODE_LINE_LOOP: mode = GL_LINE_LOOP; break;
			default: gLog("Invalid primitive mode: %d", primitive.mode); assert(false); 
		}

		glDrawElements(
				mode,
				index_accessor.count,
				index_accessor.componentType,
				(char *)NULL + index_accessor.byteOffset
				);

		for (; it != it_end; it++) {
			VertexAttributeType attribute_type = GetVertexAttributeType(it->first);
			if (attribute_type != VertexAttributeTypeCount) {
				glDisableVertexAttribArray(attribute_type);
			}
		}
	}
}

void AssetFile::DrawNode(RenderProgram &program, const tinygltf::Node &node, const Matrix44f& matrix) const {
	Matrix44f local_matrix = matrix;
	if (node.matrix.size() == 16) {
		// convert the matrix from double to float
		Matrix44f mat;
		for (int i = 0; i < 16; ++i) {
			mat.data()[i] = node.matrix.data()[i];
		}
		local_matrix *= mat; 
	} else {
		if (node.scale.size() == 3) {
			local_matrix *= ScaleMat44(node.scale[0], node.scale[1], node.scale[2]); 
		}

		if (node.rotation.size() == 4) {
			local_matrix *= Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]).toGLMatrix();
		}

		if (node.translation.size() == 3) {
			local_matrix *= TranslateMat44(node.translation[0], node.translation[1], node.translation[2]);
		}
	}

	if (node.mesh >= 0) {
		program.SetMat44("uModelMatrix", local_matrix);
		DrawMesh (mGLTFModel.meshes[node.mesh], local_matrix);
	}

	for (int i = 0; i < node.children.size(); ++i) {
		DrawNode(program, mGLTFModel.nodes[node.children[i]], local_matrix);
	}
}

void AssetFile::DrawModel(RenderProgram& program) const {
	// todo: support non-default scenes
	assert(mGLTFModel.defaultScene >= 0);
	const tinygltf::Scene &scene = mGLTFModel.scenes[mGLTFModel.defaultScene];
	for (int i = 0; i < scene.nodes.size(); ++i) {
		const tinygltf::Node &node = mGLTFModel.nodes[scene.nodes[i]];
		DrawNode(program, node, RotateMat44(90, 0.0, 0.0, 1.0));
	}
}

void AssetFile::DrawGui() {
	ImGui::Text("File: %s", mFilename.c_str());
	if (ImGui::TreeNode("Meshes")) {
		for (int i = 0, n = mGLTFModel.meshes.size(); i < n; ++i) {
			const tinygltf::Mesh& mesh = mGLTFModel.meshes[i];
			ImGui::PushID("mesh");
			if (ImGui::TreeNode((void*)(intptr_t)i, "[%d] %s", i, mesh.name.c_str())) {
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Nodes")) {
		for (int i = 0, n = mGLTFModel.nodes.size(); i < n; ++i) {
			const tinygltf::Node& node = mGLTFModel.nodes[i];
			ImGui::PushID("node");
			if (ImGui::TreeNode((void*)(intptr_t)i, "[%d] %s", i, node.name.c_str())) {
				DrawNodeGui(node);
				ImGui::TreePop();
			}
			ImGui::PopID();	
		}
		ImGui::TreePop();
	}
}

