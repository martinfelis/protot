#version 330 core

in vec3 inVertex;

uniform mat4 uModelViewProj;
uniform float uTime;

out vec2 ioUV;

void main() {
	gl_Position = uModelViewProj * vec4(inVertex, 1);
	ioUV = (inVertex.xy + vec2(1,1)) / 2.0;
}
