$input v_color0, v_path_length

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

void main()
{
	if (mod(v_path_length * 20.0, 2) < 1.0)
		discard;

	gl_FragColor = v_color0;
}
