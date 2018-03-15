#version 150 core


smooth in vec4 ioFragColor;
in vec3 ioFragPosition;

out vec4 outColor;

void main() {
	gl_FragDepth = gl_FragCoord.z;

	outColor = ioFragColor;
}
