#version 150 core

in vec4 inCoord;
in vec2 inUV;

out vec2 ioUV;

void main() {
	ioUV = inUV;
	gl_Position = inCoord;
}
