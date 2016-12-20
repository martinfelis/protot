$input a_position, a_texcoord0, a_texcoord1, a_texcoord2
$output v_color0

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

uniform vec4 u_line_params;

#define thickness u_line_params.x
#define miter u_line_params.y
#define width u_line_params.z
#define height u_line_params.w
#define current_pos a_position
#define prev_pos a_texcoord0
#define next_pos a_texcoord1
#define direction a_texcoord2

void main()
{
	float aspect = width / height;
	vec2 aspect_vec = vec2(aspect, 1.0);

  vec4 current_proj = mul(u_modelViewProj, vec4(current_pos, 1.0));
  vec4 prev_proj = mul(u_modelViewProj, vec4(prev_pos, 1.0));
  vec4 next_proj = mul(u_modelViewProj, vec4(next_pos, 1.0));

	vec2 current_screen = current_proj.xy / current_proj.w * aspect_vec;
	vec2 prev_screen = prev_proj.xy / prev_proj.w * aspect_vec;
	vec2 next_screen = next_proj.xy / next_proj.w * aspect_vec;

	float len = thickness;
	float orientation = direction;

	vec2 dir = vec2(0.0);
	if (current_screen == prev_screen) {
		dir = normalize(next_screen - current_screen);
	} else if (current_screen == next_screen) {
		dir = normalize (current_screen - prev_screen);
	} else {
		vec2 dirA = normalize (current_screen - prev_screen);
		if (miter == 1) {
      vec2 dirB = normalize(next_screen - current_screen);
      //now compute the miter join normal and length
      vec2 tangent = normalize(dirA + dirB);
      vec2 perp = vec2(-dirA.y, dirA.x);
      vec2 miter_vec = vec2(-tangent.y, tangent.x);
      dir = tangent;
      len = min (2.5 * thickness, thickness / dot(miter_vec, perp));
		} else {
			dir = dirA;
		}
	}

	vec2 normal = vec2(-dir.y, dir.x);
	normal *= len/2.0;
	normal.x /= aspect;

	vec4 offset = vec4(normal * orientation, 0.0, 0.0);
	gl_Position = current_proj + offset;
	v_color0 = vec4(1.0, 0.0, 1.0, 1.0);
}
