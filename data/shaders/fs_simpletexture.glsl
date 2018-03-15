#version 150 core

in vec2 ioUV;

out vec4 outColor;

uniform sampler2D uTexture;

void main() {
	outColor = texture(uTexture, ioUV);
}
