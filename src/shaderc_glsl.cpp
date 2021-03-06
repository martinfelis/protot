/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "shaderc.h"
#include "glsl-optimizer/src/glsl/glsl_optimizer.h"

bool compileGLSLShader(bx::CommandLine& _cmdLine, uint32_t _gles, const std::string& _code, bx::WriterI* _writer)
{
	char ch = tolower(_cmdLine.findOption('\0', "type")[0]);
	const glslopt_shader_type type = ch == 'f'
		? kGlslOptShaderFragment
		: (ch == 'c' ? kGlslOptShaderCompute : kGlslOptShaderVertex);

	glslopt_target target = kGlslTargetOpenGL;
	switch (_gles)
	{
	case BX_MAKEFOURCC('M', 'T', 'L', 0):
		target = kGlslTargetMetal;
		break;

	case 2:
		target = kGlslTargetOpenGLES20;
		break;

	case 3:
		target = kGlslTargetOpenGLES30;
		break;

	default:
		target = kGlslTargetOpenGL;
		break;
	}

	glslopt_ctx* ctx = glslopt_initialize(target);

	glslopt_shader* shader = glslopt_optimize(ctx, type, _code.c_str(), 0);

	if (!glslopt_get_status(shader) )
	{
		const char* log = glslopt_get_log(shader);
		int32_t source = 0;
		int32_t line = 0;
		int32_t column = 0;
		int32_t start = 0;
		int32_t end = INT32_MAX;

		if (3 == sscanf(log, "%u:%u(%u):", &source, &line, &column)
		&&  0 != line)
		{
			start = bx::uint32_imax(1, line-10);
			end = start + 20;
		}

		printCode(_code.c_str(), line, start, end);
		fprintf(stderr, "Error: %s\n", log);
		glslopt_cleanup(ctx);
		return false;
	}

	const char* optimizedShader = glslopt_get_output(shader);

	// Trim all directives.
	while ('#' == *optimizedShader)
	{
		optimizedShader = bx::strnl(optimizedShader);
	}

	if (0 != _gles)
	{
		char* code = const_cast<char*>(optimizedShader);
		strreplace(code, "gl_FragDepthEXT", "gl_FragDepth");

		strreplace(code, "texture2DLodEXT", "texture2DLod");
		strreplace(code, "texture2DProjLodEXT", "texture2DProjLod");
		strreplace(code, "textureCubeLodEXT", "textureCubeLod");
		strreplace(code, "texture2DGradEXT", "texture2DGrad");
		strreplace(code, "texture2DProjGradEXT", "texture2DProjGrad");
		strreplace(code, "textureCubeGradEXT", "textureCubeGrad");

		strreplace(code, "shadow2DEXT", "shadow2D");
		strreplace(code, "shadow2DProjEXT", "shadow2DProj");
	}

	UniformArray uniforms;

	{
		const char* parse = optimizedShader;

		while (NULL != parse
			&&  *parse != '\0')
		{
			parse = bx::strws(parse);
			const char* eol = strchr(parse, ';');
			if (NULL != eol)
			{
				const char* qualifier = parse;
				parse = bx::strws(bx::strword(parse) );

				if (0 == strncmp(qualifier, "attribute", 9)
				||  0 == strncmp(qualifier, "varying", 7) )
				{
					// skip attributes and varyings.
					parse = eol + 1;
					continue;
				}

				if (0 != strncmp(qualifier, "uniform", 7) )
				{
					// end if there is no uniform keyword.
					parse = NULL;
					continue;
				}

				const char* precision = NULL;
				const char* typen = parse;

				if (0 == strncmp(typen, "lowp", 4)
				||  0 == strncmp(typen, "mediump", 7)
				||  0 == strncmp(typen, "highp", 5) )
				{
					precision = typen;
					typen = parse = bx::strws(bx::strword(parse) );
				}

				BX_UNUSED(precision);

				char uniformType[256];
				parse = bx::strword(parse);

				if (0 == strncmp(typen, "sampler", 7) )
				{
					strcpy(uniformType, "int");
				}
				else
				{
					bx::strlcpy(uniformType, typen, parse-typen+1);
				}

				const char* name = parse = bx::strws(parse);

				char uniformName[256];
				uint8_t num = 1;
				const char* array = bx::strnstr(name, "[", eol-parse);
				if (NULL != array)
				{
					bx::strlcpy(uniformName, name, array-name+1);

					char arraySize[32];
					const char* end = bx::strnstr(array, "]", eol-array);
					bx::strlcpy(arraySize, array+1, end-array);
					num = atoi(arraySize);
				}
				else
				{
					bx::strlcpy(uniformName, name, eol-name+1);
				}

				Uniform un;
				un.type = nameToUniformTypeEnum(uniformType);

				if (UniformType::Count != un.type)
				{
					BX_TRACE("name: %s (type %d, num %d)", uniformName, un.type, num);

					un.name = uniformName;
					un.num = num;
					un.regIndex = 0;
					un.regCount = num;
					uniforms.push_back(un);
				}

				parse = eol + 1;
			}
		}
	}

	uint16_t count = (uint16_t)uniforms.size();
	bx::write(_writer, count);

	for (UniformArray::const_iterator it = uniforms.begin(); it != uniforms.end(); ++it)
	{
		const Uniform& un = *it;
		uint8_t nameSize = (uint8_t)un.name.size();
		bx::write(_writer, nameSize);
		bx::write(_writer, un.name.c_str(), nameSize);
		uint8_t uniformType = un.type;
		bx::write(_writer, uniformType);
		bx::write(_writer, un.num);
		bx::write(_writer, un.regIndex);
		bx::write(_writer, un.regCount);

		BX_TRACE("%s, %s, %d, %d, %d"
			, un.name.c_str()
			, getUniformTypeName(un.type)
			, un.num
			, un.regIndex
			, un.regCount
			);
	}

	uint32_t shaderSize = (uint32_t)strlen(optimizedShader);
	bx::write(_writer, shaderSize);
	bx::write(_writer, optimizedShader, shaderSize);
	uint8_t nul = 0;
	bx::write(_writer, nul);

	glslopt_cleanup(ctx);

	return true;
}
