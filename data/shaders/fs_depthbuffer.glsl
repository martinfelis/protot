#version 330 core

in vec2 uv;

out vec3 color;

uniform sampler2D rendered_texture;
uniform float near;
uniform float far;

void main() {
	float z = texture(rendered_texture, uv).r;
	float c = (2.0 * near) / (far + near - z * (far - near));
	c = 2.0 * near * far / (far + near - z * (far - near));

	color = vec3(z);
}
