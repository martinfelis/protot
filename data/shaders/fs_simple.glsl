#version 150 core

uniform vec4 uColor;

in vec4 ioFragmentColor;

out vec4 outColor;

void main() {
	outColor = vec4(ioFragmentColor.xyz * uColor.xyz * ioFragmentColor.w * uColor.w, 1.0);
}
