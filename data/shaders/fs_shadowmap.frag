#version 150 core

in vec4 ioFragColor;
in vec3 ioFragPosition;

out vec4 outColor;

void main() {
	outColor = ioFragColor;
}
