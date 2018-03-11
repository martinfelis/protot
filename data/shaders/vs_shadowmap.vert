#version 150 core
#extension GL_ARB_explicit_attrib_location : require

in vec4 inCoord;
in vec3 inNormal;
in vec2 inUV;
in vec4 inColor;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat3 uNormalMatrix;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;

smooth out vec4 ioFragColor;
out vec3 ioNormal;
out vec3 ioFragPosition;

void main() {
	gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * inCoord;
	ioFragColor = inColor;
	ioNormal = uNormalMatrix * inNormal;
	ioFragPosition = (uModelMatrix * inCoord).xyz;
}
