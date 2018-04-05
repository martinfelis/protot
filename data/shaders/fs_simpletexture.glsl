#version 150 core

in vec2 ioFragTexCoords;

out vec4 outColor;

uniform sampler2D uTexture;

void main() {
	outColor = texture(uTexture, ioFragTexCoords);
}
