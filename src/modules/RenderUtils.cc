#include <string.h> // strlen
#include <locale.h>

#include <iostream>
#include <fstream>

#include "RenderModule.h"
#include "RenderUtils.h"
#include "Globals.h"

using namespace SimpleMath;

//
// RenderProgram
//

RenderProgram::~RenderProgram() {
	if (mProgramId > 0)
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
		gLog("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", mVertexShaderFilename.c_str());
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
	gLog("Compiling shader : %s\n", mVertexShaderFilename.c_str());
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		gLog("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	gLog("Compiling shader : %s\n", mFragmentShaderFilename.c_str());
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		gLog("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	gLog("Linking program\n");
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
		gLog("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	mProgramId = ProgramID;
	return true;
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

	if (mDepthBuffer != -1) {
		glDeleteRenderbuffers(1, &mDepthBuffer);
		mDepthBuffer = -1;
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

	if (mFlags & EnableDepth) {
		glGenRenderbuffers(1, &mDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
	}

}


