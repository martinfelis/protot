#version 150 core

uniform vec4 uColor;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;

smooth in vec4 ioFragColor;
in vec3 ioNormal;
in vec3 ioFragPosition;

out vec4 outColor;

void main() {
	// ambient lighting
	float ambient_strength = 0.1;
	vec4 ambient = ambient_strength * ioFragColor;

	// diffuse lighting
	vec3 norm = normalize(ioNormal);
	vec3 light_dir = normalize(uLightDirection);
	float diff = max(dot(norm, light_dir), 0.0);
	vec4 diffuse = diff * ioFragColor;

	// specular lighting
	vec3 view_dir = normalize(uViewPosition - ioFragPosition);
	vec3 reflect_dir = reflect(-light_dir, norm);

	float specular_strength = 0.5;
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
	vec4 specular = specular_strength * spec * vec4(1.0, 1.0, 1.0, 1.0);

	outColor = ambient + diffuse + specular;
}
