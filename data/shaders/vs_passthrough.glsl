#version 330 core

in vec3 vertex_position_modelspace;

out vec2 uv;

void main() {
	gl_Position = vec4(vertex_position_modelspace, 1);
	uv = (vertex_position_modelspace.xy + vec2(1,1)) / 2.0;
}
