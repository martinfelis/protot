#version 150 core

uniform vec4 uColor;
uniform vec3 uLightDirection;

smooth in vec4 ioFragColor;
in vec3 ioNormal;

out vec4 outColor;

void main() {
	float ambient_strength = 0.1;

	vec4 ambient = ambient_strength * ioFragColor;

	vec3 norm = normalize(ioNormal);
	vec3 light_dir = normalize(uLightDirection);
	float diff = max(dot(norm, light_dir), 0.0);
	vec4 diffuse = diff * ioFragColor;

	outColor = ambient + diffuse;
}
