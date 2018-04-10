#version 150 core

uniform sampler2D uColor;
uniform sampler2D uNormal;
uniform sampler2D uDepth;

#define USE_SAMPLER2D_SHADOW 1

#ifdef USE_SAMPLER2D_SHADOW
uniform sampler2DShadow uShadowMap;
#else
uniform sampler2D uShadowMap;
#endif

uniform sampler2D uPosition;
uniform vec3 uViewPosition;

uniform vec3 uLightDirection;
uniform mat4 uLightSpaceMatrix;

in vec2 ioFragTexCoords;

out vec3 outColor;

void main() {
	vec3 color = texture(uColor, ioFragTexCoords).rgb;
	vec3 normal = texture (uNormal, ioFragTexCoords).xyz;
	float depth = texture (uDepth, ioFragTexCoords).r;
	vec3 position = texture (uPosition, ioFragTexCoords).xyz;

	// ambient lighting
	float ambient_strength = 0.2;
	vec3 ambient = ambient_strength * color;

	vec3 light_dir = normalize(uLightDirection);

	// diffuse lighting
	float diff = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = diff * color;

	// specular lighting
	vec3 view_dir = normalize(-position); 
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float spec = pow(max(dot(normal, halfway_dir), 0.0), 32);
	vec3 specular = spec * vec3(0.5);

	// shadow
	outColor = ambient + (diffuse + specular);
}
