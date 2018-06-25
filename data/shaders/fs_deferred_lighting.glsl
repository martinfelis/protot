#version 150 core

uniform sampler2D uColor;
uniform sampler2D uNormal;
uniform sampler2D uDepth;
uniform sampler2D uAmbientOcclusion;

#define USE_SAMPLER2D_SHADOW 1

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMap;
#else
uniform sampler2D uShadowMap;
#endif

uniform sampler2D uPosition;

uniform vec3 uLightDirection;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uViewToLightSpaceMatrix;
uniform float uShadowBias;

in vec2 ioFragTexCoords;

out vec3 outColor;

float ShadowCalculationPCF(vec4 frag_pos_light_space, vec3 frag_normal_light_space) {
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	if (abs(projected_coordinates.z) > 1.0 ) {
		return 1.0;
	}

	float current_depth = projected_coordinates.z;

	float bias = 0.00;
	bias = max(0.001 * (1.0 - dot(frag_normal_light_space, uLightDirection)), uShadowBias);

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

float ShadowCalculation(vec4 frag_pos_light_space, vec3 frag_normal_light_space) {
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	float current_depth = projected_coordinates.z;

	float bias = 0.01;

#ifdef USE_SAMPLER2D_SHADOW
	float closest_depth = texture(uShadowMap, vec3(projected_coordinates.xy, current_depth - bias));
#else
	float closest_depth = texture(uShadowMap, projected_coordinates.xy).r;
	bias = max(0.005 * (1.0 - dot(frag_normal_light_space, uLightDirection)), 0.003);
#endif

	return current_depth - bias > closest_depth ? 1.0 : 0.0;
}

void main() {
	vec3 color = texture(uColor, ioFragTexCoords).rgb;
	vec3 normal = texture (uNormal, ioFragTexCoords).xyz;
	float depth = texture (uDepth, ioFragTexCoords).r;
	vec3 position = texture (uPosition, ioFragTexCoords).xyz;
	float ambient_occlusion = texture(uAmbientOcclusion, ioFragTexCoords).r;

	// ambient lighting
	float ambient_strength = 0.2;
	vec3 ambient = ambient_strength * color;

	vec3 light_dir = uLightDirection;

	// diffuse lighting
	float diff = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = diff * color;

	// specular lighting
	vec3 view_dir = normalize(-position); 
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float spec = pow(max(dot(normal, halfway_dir), 0.0), 32);
	vec3 specular = spec * vec3(0.5);

	// shadow (need to transform position and normal to light space)
	vec4 position_light_space = uViewToLightSpaceMatrix * vec4(position, 1.0);
	vec3 normal_light_space = (transpose(inverse(uViewToLightSpaceMatrix)) * vec4(normal, 1.0)).xyz;
	float shadow = ShadowCalculationPCF(position_light_space, normal);

	outColor = (ambient * ambient_occlusion + (1.0 - shadow) * (diffuse + specular)) * ambient_occlusion;
//	outColor = (ambient + (diffuse + specular)) * ambient_occlusion;
}
