#version 150 core
#extension GL_ARB_explicit_uniform_location : require

in vec3 inVertex;

uniform mat4 uModelViewProj;

void main() {
	gl_Position = uModelViewProj * vec4(inVertex, 1);
}
