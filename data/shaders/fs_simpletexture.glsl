#version 330 core

in vec2 uv;

out vec3 color;

uniform sampler2D rendered_texture;
uniform float time;

void main() {
	color = texture(rendered_texture, uv 
	+ 0.00 * vec2(
		sin(time + 1024.0 * uv.x),
		cos(time + 768.0  * uv.y)
		)
	).xyz;
	color = vec3((texture(rendered_texture, uv).x + 1) * 0.5 );
//	* 1 * (cos(time * 0.3) * 0.5) + 0.5);
}
