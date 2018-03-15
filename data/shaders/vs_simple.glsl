#version 150 core
#extension GL_ARB_explicit_attrib_location : require

in vec4 inCoord;
in vec3 inNormal;
in vec2 inUV;
in vec4 inColor;

uniform mat4 uModelViewProj;

out vec4 ioFragmentColor;

void main() {
	gl_Position = uModelViewProj * inCoord;
	ioFragmentColor = inColor;
}
