#version 150 core
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 inCoord;
layout(location = 1) in vec3 inColor;

uniform mat4 uModelViewProj;

out vec3 fragmentColor;

void main() {
	gl_Position = uModelViewProj * vec4(inCoord, 1);
	fragmentColor = inColor;
}
