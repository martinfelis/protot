#version 150 core
#extension GL_ARB_explicit_attrib_location : require

in vec4 inCoord;
in vec3 inNormal;
in vec2 inUV;
in vec4 inColor;

uniform mat4 uModelViewProj;
uniform vec3 uLightDirection;

smooth out vec4 ioFragColor;
out vec3 ioNormal;

void main() {
	gl_Position = uModelViewProj * inCoord;
	ioFragColor = inColor;
	ioNormal = inNormal;
}
