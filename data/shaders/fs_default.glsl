#version 150 core

uniform vec4 uColor;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;
uniform sampler2D uAlbedoTexture;

#define USE_SAMPLER2D_SHADOW 1

//
// Single Shadow MAp
//

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMap;
#else
uniform sampler2D uShadowMap;
#endif

//
// Cascaded Shadow Maps
//

const int NUM_SPLITS = 4;

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMaps[NUM_SPLITS];
#else
uniform sampler2D uShadowMaps[NUM_SPLITS];
#endif

uniform mat4 uViewToLightMatrix[NUM_SPLITS];
uniform float uShowCascadesAlpha;
uniform vec4 uShadowSplits;
uniform vec4 uShadowSplitBias;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;

in vec3 ioFragPosition;
in vec3 ioFragNormal;
in vec2 ioFragTexCoords;
smooth in vec4 ioFragColor;
in vec4 ioFragPosLightSpace;

out vec4 outColor;
out vec3 outPosition;
out vec3 outNormal;

float ShadowCalculationPCF_OLD(vec4 frag_pos_light_space) {
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

#ifdef USE_SAMPLER2D_SHADOW
float ShadowCalculationPCF(sampler2DShadow shadow_map, vec4 frag_pos_light_space, vec3 frag_normal_light_space, float shadow_bias) {
#else
float ShadowCalculationPCF(sampler2D shadow_map, vec4 frag_pos_light_space, vec3 frag_normal_light_space, float shadow_bias) {
#endif
	vec3 projected_coordinates = frag_pos_light_space.xyz / frag_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 + 0.5;

	if (abs(projected_coordinates.z) > 1.0 ) {
		return 0.0;
	}

	float current_depth = projected_coordinates.z;

	float bias = 0.00;
	bias = max(shadow_bias * (1.0 - dot(frag_normal_light_space, uLightDirection)), shadow_bias);

	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
#ifdef USE_SAMPLER2D_SHADOW
			vec2 coordinate = projected_coordinates.xy + vec2(x, y) * texel_size;
			float pcf_depth = texture(shadow_map, vec3(coordinate, current_depth - bias));
#else
			float pcf_depth = texture(shadow_map, projected_coordinates.xy).r;
#endif
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
		}
	}

	shadow /= 9.0;

	return shadow;
}

vec3 get_cascade_color (float depth) {
	if (depth < uShadowSplits[0]) {
		return vec3 (1.0, 0.0, 0.0);
	} else if (depth < uShadowSplits[1]) {
		return vec3 (1.0, 1.0, 0.0);
	} else if (depth < uShadowSplits[2]) {
		return vec3 (0.0, 1.0, 0.0);
	}

	return vec3 (0.0, 0.0, 1.0);
}

void main() {
	vec4 albedo_color = texture(uAlbedoTexture, ioFragTexCoords) * ioFragColor * uColor;
	vec3 position = ioFragPosition.xyz;

	// ambient lighting
	float ambient_strength = 0.2;
	vec4 ambient = ambient_strength * albedo_color;

	// diffuse lighting
	vec3 normal = normalize(ioFragNormal);
	vec3 light_dir = -(mat3(uViewMatrix)) * uLightDirection;
	float diff = max(dot(normal, light_dir), 0.0);
	vec4 diffuse = diff * albedo_color;

	// specular lighting
	vec3 view_dir = normalize(-ioFragPosition);
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float spec = pow(max(dot(normal, halfway_dir), 0.0), 32);
	vec4 specular = spec * vec4(0.5);

	// shadow
	float shadow = 0;
	if (-position.z < uShadowSplits[0]) {
		// shadow (need to transform position and normal to light space)
		vec4 position_light_space = uViewToLightMatrix[0] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[0])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMaps[0], position_light_space, normal, uShadowSplitBias[0]);
	} else if (-position.z< uShadowSplits[1]) {
		vec4 position_light_space = uViewToLightMatrix[1] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[1])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMaps[1], position_light_space, normal, uShadowSplitBias[1]);
	} else if (-position.z< uShadowSplits[2]) {
		vec4 position_light_space = uViewToLightMatrix[2] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[2])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMaps[2], position_light_space, normal, uShadowSplitBias[2]);
	} else {
		vec4 position_light_space = uViewToLightMatrix[3] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[3])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMaps[3], position_light_space, normal, uShadowSplitBias[3]);
	}

	vec4 cascade = vec4(get_cascade_color(-position.z), 1.0);
	ambient = (uShowCascadesAlpha * cascade) + (1.0 - uShowCascadesAlpha) * ambient;
  outColor = (ambient + (1.0 - shadow) * (diffuse + specular));

//	float shadow = ShadowCalculationPCF(uShadowMap, ioFragPosLightSpace, normal, 0.001);
	outColor = ambient + (1.0 - shadow) * (diffuse + specular);

	outPosition = ioFragPosition.xyz;
	outNormal = normalize(ioFragNormal);
}
