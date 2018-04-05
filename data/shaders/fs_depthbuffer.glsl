#version 150 core

in vec2 ioFragTexCoords;

out vec3 outColor;

uniform sampler2D uDepthTexture;
uniform float uIsOrthographic;
uniform float uNear;
uniform float uFar;

void main() {
	float z = texture(uDepthTexture, ioFragTexCoords).r;
	float c;
	if (uIsOrthographic == 1.0) {
		c = z;
	} else {
		c = (2.0 * uNear) / (uFar + uNear - z * (uFar - uNear));
	}

	outColor = vec3(c);
}
