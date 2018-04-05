#version 150 core

in vec2 ioFragTexCoords;

out vec3 outColor;

uniform sampler2D uPositions;
uniform sampler2D uNormals;
uniform sampler2D uNoise;

uniform float uRadius;
uniform float uBias;
uniform int uSampleCount;
uniform vec3 uSamples[64];
uniform mat4 uProjection;

void main() {
	vec2 noise_scale = textureSize(uPositions, 0) / 2.0;

	vec3 position = texture(uPositions, ioFragTexCoords).xyz;
	vec3 normal = normalize(texture(uNormals, ioFragTexCoords)).rgb;
	vec3 random_vector = normalize(texture(uNoise, ioFragTexCoords * noise_scale).xyz);

	vec3 tangent = normalize(random_vector - normal * dot(random_vector, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3 (tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < uSampleCount; ++i) {
		vec3 sample = TBN * uSamples[i];
		sample = position + sample * uRadius;

		vec4 offset = vec4(sample, 1.0);
		offset = uProjection * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;
		float sample_depth = texture(uPositions, offset.xy).z;

		float range_check = smoothstep(0.0, 1.0, uRadius / abs(position.z - sample_depth));
		occlusion += (sample_depth >= sample.z + uBias ? 1.0 : 0.0) * range_check; 
	}

	occlusion = 1.0 - (occlusion / uSampleCount);
	outColor = vec3(occlusion);	
}
