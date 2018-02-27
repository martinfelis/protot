#version 150 core

uniform vec4 uColor;

in vec3 fragmentColor;

out vec3 outColor;

void main() {
	outColor = vec3(
		uColor.r * fragmentColor.r,
		uColor.g * fragmentColor.g,
		uColor.b * fragmentColor.b
		);

	outColor = max(uColor.rgb, fragmentColor);
//outColor = fragmentColor + uColor.rgb - uColor.rgb;
}
