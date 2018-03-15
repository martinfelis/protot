#version 150 core

in vec4 inCoord;
in vec4 inColor;

uniform mat4 uModelMatrix;
uniform mat4 uLightSpaceMatrix;

smooth out vec4 ioFragColor;

void main() {
	gl_Position = uLightSpaceMatrix * uModelMatrix * inCoord;
	ioFragColor = inColor;
}
