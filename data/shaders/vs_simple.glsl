#version 150 core
#extension GL_ARB_explicit_uniform_location : require

in vec3 vertexPosition_modelspace;

void main() {
	gl_Position.xyz = vertexPosition_modelspace;
	gl_Position.w = 1.0;
}
