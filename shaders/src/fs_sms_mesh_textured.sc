$input v_view, v_normal, v_shadowcoord, v_texcoord0

/*
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

#define SHADOW_PACKED_DEPTH 0
#define TEXTURED 1
#include "fs_sms_shadow.sh"
