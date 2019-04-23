#version 150 core

uniform sampler2D uPosition;
uniform sampler2D uDepth;
uniform sampler2D uNormal;
uniform sampler2D uColor;
uniform sampler2D uAmbientOcclusion;

const int NUM_SPLITS = 4;

#define USE_SAMPLER2D_SHADOW 1

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMap[NUM_SPLITS];
#else
uniform sampler2D uShadowMaps[NUM_SPLITS];
#endif

uniform mat4 uViewToLightMatrix[NUM_SPLITS];

uniform float uNear;
uniform float uFar;
uniform float uShowCascadesAlpha;
uniform vec4 uShadowSplits;
uniform vec4 uShadowSplitBias;
uniform vec3 uLightDirection;
uniform float uShadowBias;

in vec2 ioFragTexCoords;

out vec3 outColor;

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
	vec3 color = texture(uColor, ioFragTexCoords).rgb;
	vec3 normal = texture (uNormal, ioFragTexCoords).xyz;
	float depth = texture (uDepth, ioFragTexCoords).r;
	vec3 position = texture (uPosition, ioFragTexCoords).xyz;
	float ambient_occlusion = texture(uAmbientOcclusion, ioFragTexCoords).r;

	// ambient lighting
	float ambient_strength = 0.2;
	vec3 ambient = ambient_strength * color;

	vec3 light_dir = -uLightDirection;

	// diffuse lighting
	float diff = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = diff * color;

	// specular lighting
	vec3 view_dir = normalize(-position); 
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float spec = pow(max(dot(normal, halfway_dir), 0.0), 32);
	vec3 specular = spec * vec3(0.5);

	float shadow = 0;
	float normalized_depth = (depth - uNear) / (uFar - uNear);

	if (-position.z < uShadowSplits[0]) {
		// shadow (need to transform position and normal to light space)
		vec4 position_light_space = uViewToLightMatrix[0] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[0])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMap[0], position_light_space, normal, uShadowSplitBias[0]);
	} else if (-position.z< uShadowSplits[1]) {
		vec4 position_light_space = uViewToLightMatrix[1] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[1])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMap[1], position_light_space, normal, uShadowSplitBias[1]);
	} else if (-position.z< uShadowSplits[2]) {
		vec4 position_light_space = uViewToLightMatrix[2] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[2])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMap[2], position_light_space, normal, uShadowSplitBias[2]);
	} else {
		vec4 position_light_space = uViewToLightMatrix[3] * vec4(position, 1.0);
		vec3 normal_light_space = (transpose(inverse(uViewToLightMatrix[3])) * vec4(normal, 1.0)).xyz;
		shadow = ShadowCalculationPCF(uShadowMap[3], position_light_space, normal, uShadowSplitBias[3]);
	}

	vec3 cascade = get_cascade_color(-position.z);
	ambient = (uShowCascadesAlpha * cascade) + (1.0 - uShowCascadesAlpha) * ambient;
  outColor = (ambient + (1.0 - shadow) * (diffuse + specular)) * ambient_occlusion;

//	outColor = diffuse;
//	outColor = (ambient + (diffuse + specular)) * ambient_occlusion;
}
