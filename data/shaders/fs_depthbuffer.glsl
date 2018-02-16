#version 330 core

in vec2 ioUV;

out vec3 outColor;

uniform sampler2D uTexture;
uniform float uNear;
uniform float uFar;

void main() {
	float z = texture(uTexture, ioUV).r;
	float c = (2.0 * uNear) / (uFar + uNear - z * (uFar - uNear));
//	c = 2.0 * uNear * uFar / (uFar + uNear - z * (uFar - uNear));
//	c = (uNear + (z - uNear) / (uFar - uNear);

	c = (z - uNear) / (uFar - uNear);

	outColor = vec3(c);

	if (abs(c + 1) < 0.2)
		outColor = vec3(1, 0, 0);
//	if (abs(c - 0.1) < 0.1)
//		outColor = vec3(0, 0, 1);
//	if (abs(c - 0.8) < 0.1)
//		outColor = vec3(1, 0, 0);


//     n ---- z ------- f
//     0                1
//
//		 (n + (z - n)) (f - n)
//
//		 (f - n) 
//	outColor = vec3(z);
}
