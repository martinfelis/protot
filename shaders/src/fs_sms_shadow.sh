/*
 * Copyright 2013-2014 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_color;

uniform vec4 u_shadowMapParams;
#define u_shadowMapSize u_shadowMapParams.x
#define u_shadowMapBias u_shadowMapParams.y

#if SHADOW_PACKED_DEPTH
SAMPLER2D(u_shadowMap, 0);
#	define Sampler sampler2D
#else
SAMPLER2DSHADOW(u_shadowMap, 0);
#	define Sampler sampler2DShadow
#endif // SHADOW_PACKED_DEPTH

#if TEXTURED
SAMPLER2D(sceneDefaultTexture, 1);
#endif

vec2 lit(vec3 _ld, vec3 _n, vec3 _vd, float _exp)
{
	//diff
	float ndotl = dot(_n, _ld);

	//spec
	vec3 r = 2.0*ndotl*_n - _ld; //reflect(_ld, _n);
	float rdotv = dot(r, _n);
	float spec = step(0.0, ndotl) * pow(max(0.0, rdotv), _exp) * (2.0 + _exp)/8.0;

	return max(vec2(ndotl, spec), 0.0);
}

float hardShadow(Sampler _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;
#if SHADOW_PACKED_DEPTH
	return step(texCoord.z-_bias, unpackRgbaToFloat(texture2D(_sampler, texCoord.xy) ) );
#else
	return shadow2D(_sampler, vec3(texCoord.xy, texCoord.z-_bias) );
#endif // SHADOW_PACKED_DEPTH
}

float PCF(Sampler _sampler, vec4 _shadowCoord, float _bias, vec2 _texelSize)
{
	vec2 texCoord = _shadowCoord.xy/_shadowCoord.w;

	bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
				|| any(lessThan   (texCoord, vec2_splat(0.0)))
				 ;

	if (outside)
	{
		return 1.0;
	}

	float result = 0.0;
	vec2 offset = _texelSize * _shadowCoord.w;

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-1.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(-0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += hardShadow(_sampler, _shadowCoord + vec4(vec2(1.5,  1.5) * offset, 0.0, 0.0), _bias);

	return result / 16.0;
}

void main()
{
	vec3 color = u_color.xyz;

	vec3 v  = v_view;
	vec3 vd = -normalize(v);
	vec3 n  = v_normal;
	vec3 l  = u_lightPos.xyz;
	vec3 ld = normalize(l);

	vec2 lc = lit(ld, n, vd, 1.0);

	vec2 texelSize = vec2_splat(1.0/u_shadowMapSize);
	float visibility = PCF(u_shadowMap, v_shadowcoord, u_shadowMapBias, texelSize);	

	vec3 ambient = 0.05 * color;
	vec3 brdf = (lc.x * color + lc.y * color)  * visibility;

#if TEXTURED
  vec4 texcolor = toLinear (texture2D(sceneDefaultTexture, v_texcoord0) );
	brdf = brdf * texcolor.xyz;
	ambient = ambient * texcolor.xyz;
#endif
	vec3 final = toGamma(abs(ambient + brdf) );

	gl_FragColor = vec4(final, 1.0);
}
