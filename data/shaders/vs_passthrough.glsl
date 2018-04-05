#version 150 core

in vec4 inCoord;
in vec2 inUV;

out vec2 ioFragTexCoords;

void main() {
	ioFragTexCoords = inUV;
	gl_Position = inCoord;
}
