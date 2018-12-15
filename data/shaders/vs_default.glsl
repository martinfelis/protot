#version 150 core

in vec4 inCoord;
in vec3 inNormal;
in vec2 inUV;
in vec4 inColor;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uLightSpaceMatrix;
uniform mat3 uNormalMatrix;
uniform vec3 uLightDirection;
uniform vec3 uViewPosition;

out vec3 ioFragPosition;
out vec3 ioFragNormal;
out vec2 ioFragTexCoords;
smooth out vec4 ioFragColor;
out vec4 ioFragPosLightSpace;

void main() {
	mat4 model_view_matrix = uViewMatrix * uModelMatrix;

	ioFragPosition = (model_view_matrix* inCoord).xyz;
	ioFragNormal = transpose(inverse(mat3(model_view_matrix))) * inNormal;
	ioFragTexCoords = inUV;
	ioFragColor = inColor;
	ioFragPosLightSpace = uLightSpaceMatrix * uModelMatrix * inCoord;

	gl_Position = uProjectionMatrix * model_view_matrix * inCoord;
}
