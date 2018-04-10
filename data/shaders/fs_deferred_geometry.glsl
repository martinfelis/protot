#version 150 core

uniform vec4 uColor;
uniform sampler2D uAlbedoTexture;

in vec3 ioFragPosition;
in vec3 ioFragNormal;
in vec2 ioFragTexCoords;
in vec4 ioFragColor;

out vec4 outColor;
out vec3 outNormal;
out vec3 outPosition;

void main() {
	outColor = texture(uAlbedoTexture, ioFragTexCoords) * ioFragColor * uColor;
	outNormal = normalize(ioFragNormal);
	outPosition = ioFragPosition.xyz;
}
