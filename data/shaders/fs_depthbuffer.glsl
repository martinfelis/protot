#version 330 core

in vec2 ioUV;

out vec3 outColor;

uniform sampler2D uTexture;
uniform float uNear;
uniform float uFar;

void main() {
	float z = texture(uTexture, ioUV).r;
	float c = (z - uNear) / (uFar - uNear);

	outColor = vec3(c);
}
