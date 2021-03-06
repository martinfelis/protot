$input v_color0
/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

uniform vec4 u_color;

void main()
{
	gl_FragColor = u_color * v_color0;
}
