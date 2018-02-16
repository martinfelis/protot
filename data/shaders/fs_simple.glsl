#version 330 core

layout(location = 0) out vec3 outColor;

uniform vec4 uColor;

void main() {
	outColor = uColor.rgb; 
}
