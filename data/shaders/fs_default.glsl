#version 150 core

uniform vec4 uColor;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;
uniform sampler2D uAlbedoTexture;

#define USE_SAMPLER2D_SHADOW 1

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMap;
#else
uniform sampler2D uShadowMap;
#endif

in vec3 ioFragPosition;
in vec3 ioFragNormal;
in vec2 ioFragTexCoords;
smooth in vec4 ioFragColor;
in vec4 ioFragPosLightSpace;

out vec4 outColor;
out vec3 outPosition;
out vec3 outNormal;

float ShadowCalculationPCF(vec4 frag_pos_light_space) {
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	float current_depth = projected_coordinates.z;

	float bias = 0.00;
	bias = max(0.005 * (1.0 - dot(ioFragNormal, uLightDirection)), 0.003);

	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(uShadowMap, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
#ifdef USE_SAMPLER2D_SHADOW
			vec2 coordinate = projected_coordinates.xy + vec2(x, y) * texel_size;
			float pcf_depth = texture(uShadowMap, vec3(coordinate, current_depth - bias));
#else
			float pcf_depth = texture(uShadowMap, projected_coordinates.xy).r;
#endif
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
		}
	}

	shadow /= 9.0;

	return shadow;
}

float ShadowCalculation(vec4 frag_pos_light_space) {
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	float current_depth = projected_coordinates.z;

	float bias = 0.01;

#ifdef USE_SAMPLER2D_SHADOW
	float closest_depth = texture(uShadowMap, vec3(projected_coordinates.xy, current_depth - bias));
#else
	float closest_depth = texture(uShadowMap, projected_coordinates.xy).r;
	bias = max(0.005 * (1.0 - dot(ioFragNormal, uLightDirection)), 0.003);
#endif

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
	float shadow = ShadowCalculationPCF(ioFragPosLightSpace);
	outColor = ambient + (1.0 - shadow) * (diffuse + specular);

	outPosition = ioFragPosition.xyz;
	outNormal = ioFragNormal;
}
