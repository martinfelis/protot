#version 150 core

in vec2 ioFragTexCoords;

out vec3 outColor;

uniform sampler2D uAmbientOcclusion;
uniform int uBlurSize;

float box_blur_occlusion (sampler2D ambient_occlusion, vec2 tex_coord, int size) {
	vec2 texel_size = 1.0 / vec2(textureSize(uAmbientOcclusion, 0));

	float value = 0.;
	int count = 0;
	for (int i = -size; i < size; i++) {
		for (int j = -size; j < size; j++) {
			value += texture(ambient_occlusion, tex_coord + vec2(i,j) * texel_size).x;
			count++;
		}
	}

	return value / (4 * size * size);
}

void main() {
	float occlusion;
	if (uBlurSize > 0) {
		occlusion = box_blur_occlusion(uAmbientOcclusion, ioFragTexCoords, uBlurSize);
	} else {
		occlusion = texture(uAmbientOcclusion, ioFragTexCoords).x;
	}

	outColor = vec3(occlusion);
}
