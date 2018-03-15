#version 150 core

uniform vec4 uColor;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;
uniform sampler2D uAlbedoTexture;
uniform sampler2D uShadowMap;

in vec3 ioFragPosition;
in vec3 ioFragNormal;
in vec2 ioFragTexCoords;
smooth in vec4 ioFragColor;
in vec4 ioFragPosLightSpace;

out vec4 outColor;

float ShadowCalculation(vec4 frag_pos_light_space) {
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	float closest_depth = texture(uShadowMap, projected_coordinates.xy).r;
	float current_depth = projected_coordinates.z;

	float bias = max(0.05 * (1.0 - dot(ioFragNormal, uLightDirection)), 0.005);
	bias = 0.0;
	return current_depth - bias > closest_depth ? 1.0 : 0.0;
}

void main() {
	vec4 albedo_color = texture(uAlbedoTexture, ioFragTexCoords) * ioFragColor * uColor;

	// ambient lighting
	float ambient_strength = 0.2;
	vec4 ambient = ambient_strength * albedo_color;

	// diffuse lighting
	vec3 normal_dir = normalize(ioFragNormal);
	vec3 light_dir = normalize(uLightDirection);
	float diff = max(dot(normal_dir, light_dir), 0.0);
	vec4 diffuse = diff * albedo_color;

	// specular lighting
	vec3 view_dir = normalize(uViewPosition - ioFragPosition);
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float spec = pow(max(dot(normal_dir, halfway_dir), 0.0), 32);
	vec4 specular = spec * vec4(0.5);

	// shadow
	float shadow = ShadowCalculation(ioFragPosLightSpace);
	outColor = ambient + (1.0 - shadow) * (diffuse + specular);
//
//
//	vec3 projected_coordinates = ioFragPosLightSpace.xyz / ioFragPosLightSpace.w;
//	projected_coordinates = projected_coordinates * 0.5 + 0.5;
//	float shadow_map_value = texture(uShadowMap, projected_coordinates.xy).r;
//	outColor = shadow_map_value * vec4(1.0, 1.0, 1.0, 1.0);
//
//	outColor = vec4(vec3(1.0f - shadow_map_value), 1.0);

//	outColor = (shadow) * vec4(1.0, 1.0, 1.0, 1.0);
//	outColor = ioFragPosLightSpace / ioFragPosLightSpace.w;
//	outColor = ambient + diffuse + specular;
}
