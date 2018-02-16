#version 330 core

in vec2 ioUV;

out vec3 outColor;

uniform sampler2D uTexture;
uniform float uTime;

void main() {
	outColor = texture(uTexture, ioUV 
	+ 0.00 * vec2(
		sin(uTime + 1024.0 * ioUV.x),
		cos(uTime + 768.0  * ioUV.y)
		)
	).xyz;
}
