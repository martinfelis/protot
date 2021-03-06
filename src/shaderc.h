/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef SHADERC_H_HEADER_GUARD
#define SHADERC_H_HEADER_GUARD

#include <bx/macros.h>

#ifndef SHADERC_CONFIG_HLSL
#	define SHADERC_CONFIG_HLSL BX_PLATFORM_WINDOWS
#endif // SHADERC_CONFIG_HLSL

extern bool g_verbose;

#include <alloca.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

#include <bx/bx.h>
#include <bx/debug.h>
#include <bx/commandline.h>
#include <bx/endian.h>
#include <bx/uint32_t.h>
#include <bx/readerwriter.h>
#include <bx/string.h>
#include <bx/hash.h>
#include <bx/crtimpl.h>
#include "../../src/vertexdecl.h"

class LineReader
{
public:
	LineReader(const char* _str)
		: m_str(_str)
		, m_pos(0)
		, m_size((uint32_t)strlen(_str))
	{
	}

	std::string getLine()
	{
		const char* str = &m_str[m_pos];
		skipLine();

		const char* eol = &m_str[m_pos];

		std::string tmp;
		tmp.assign(str, eol - str);
		return tmp;
	}

	bool isEof() const
	{
		return m_str[m_pos] == '\0';
	}

	void skipLine()
	{
		const char* str = &m_str[m_pos];
		const char* nl = bx::strnl(str);
		m_pos += (uint32_t)(nl - str);
	}

	const char* m_str;
	uint32_t m_pos;
	uint32_t m_size;
};

struct UniformType
{
	enum Enum
	{
		Int1,
		End,

		Vec4,
		Mat3,
		Mat4,

		Count
	};
};

#define BGFX_UNIFORM_FRAGMENTBIT UINT8_C(0x10)
#define BGFX_UNIFORM_SAMPLERBIT  UINT8_C(0x20)

const char* getUniformTypeName(UniformType::Enum _enum);
UniformType::Enum nameToUniformTypeEnum(const char* _name);

struct Uniform
{
	std::string name;
	UniformType::Enum type;
	uint8_t num;
	uint16_t regIndex;
	uint16_t regCount;
};

typedef std::vector<Uniform> UniformArray;

void printCode(const char* _code, int32_t _line = 0, int32_t _start = 0, int32_t _end = INT32_MAX);
void strreplace(char* _str, const char* _find, const char* _replace);
int32_t writef(bx::WriterI* _writer, const char* _format, ...);
void writeFile(const char* _filePath, const void* _data, int32_t _size);

bool compileHLSLShader(bx::CommandLine& _cmdLine, uint32_t _d3d, const std::string& _code, bx::WriterI* _writer, bool firstPass = true);
bool compileGLSLShader(bx::CommandLine& _cmdLine, uint32_t _gles, const std::string& _code, bx::WriterI* _writer);

int compileShader(bx::CommandLine& _cmdLine, bx::ReaderSeekerI* _reader, bx::WriterI* _writer);

#endif // SHADERC_H_HEADER_GUARD
