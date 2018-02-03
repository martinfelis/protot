PROJECT (BGFX CXX)

FIND_PACKAGE (OpenGL)
FIND_PACKAGE (Threads)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#ADD_DEFINITIONS ("BX_PLATFORM_LINUX")
#ADD_DEFINITIONS ("-DBGFX_CONFIG_MULTITHREADED=0")
ADD_DEFINITIONS ("-DBGFX_CONFIG_RENDERER_OPENGL=1")
ADD_DEFINITIONS ("-DENTRY_CONFIG_USE_GLFW=1")

ADD_DEFINITIONS ("-D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS")

INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_SOURCE_DIR}/../bx/include
	${CMAKE_CURRENT_SOURCE_DIR}/../bx/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/../bimg/include
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/khronos
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/examples/common
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${OPENGL_INCLUDE_DIR}

	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fcpp/

	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glsl-optimizer/include/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glsl-optimizer/src/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glsl-optimizer/src/glsl/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glsl-optimizer/src/mesa/

	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glslang/glslang/Public/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glslang/glslang/Include/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glslang/

	${CMAKE_CURRENT_BINARY_DIR}/src


	)

SET (BGFX_SOURCES
	../bx/src/amalgamated.cpp
	src/amalgamated.cpp
	)

SET (BGFX_AUX_SOURCES
	examples/common/nanovg/nanovg.cpp
	examples/common/nanovg/nanovg_bgfx.cpp
	examples/common/imgui/imgui.cpp
	3rdparty/ib-compress/indexbuffercompression.cpp
	3rdparty/ib-compress/indexbufferdecompression.cpp
	3rdparty/ocornut-imgui/imgui.cpp
	3rdparty/ocornut-imgui/imgui_draw.cpp
	3rdparty/ocornut-imgui/imgui_demo.cpp
	)

ADD_LIBRARY ( bgfx SHARED ${BGFX_SOURCES} )
ADD_LIBRARY ( bgfx_aux SHARED ${BGFX_AUX_SOURCES} )

TARGET_LINK_LIBRARIES (bgfx
	${OPENGL_LIBRARIES}
	)

#
# shaderc compilation
#

file(GLOB glcpp-library_sources
	3rdparty/glsl-optimizer/src/mesa/main/*.c
	3rdparty/glsl-optimizer/src/mesa/program/*.c
	3rdparty/glsl-optimizer/src/glsl/glcpp/*.c
	3rdparty/glsl-optimizer/src/glsl/*.cpp
	3rdparty/glsl-optimizer/src/glsl/*.c
	3rdparty/glsl-optimizer/src/util/*.c
	)

list(REMOVE_ITEM glcpp-library_sources
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glsl-optimizer/src/glsl/main.cpp)

SET ( glslang_SRCS
	3rdparty/glslang/OGLCompilersDLL/InitializeDll.cpp
	3rdparty/glslang/SPIRV/SPVRemapper.cpp
	3rdparty/glslang/SPIRV/InReadableOrder.cpp
	3rdparty/glslang/SPIRV/SpvBuilder.cpp
	3rdparty/glslang/SPIRV/doc.cpp
	3rdparty/glslang/SPIRV/disassemble.cpp
	3rdparty/glslang/SPIRV/GlslangToSpv.cpp
	3rdparty/glslang/SPIRV/Logger.cpp
	#	3rdparty/glslang/StandAlone/StandAlone.cpp
	#	3rdparty/glslang/StandAlone/ResourceLimits.cpp
	#	3rdparty/glslang/StandAlone/spirv-remap.cpp
	#	3rdparty/glslang/hlsl/hlslGrammar.cpp
	#	3rdparty/glslang/hlsl/hlslParseHelper.cpp
	#	3rdparty/glslang/hlsl/hlslParseables.cpp
	#	3rdparty/glslang/hlsl/hlslTokenStream.cpp
	#	3rdparty/glslang/hlsl/hlslAttributes.cpp
	#	3rdparty/glslang/hlsl/hlslOpMap.cpp
	#	3rdparty/glslang/hlsl/hlslScanContext.cpp
	3rdparty/glslang/glslang/OSDependent/Unix/ossource.cpp
	#	3rdparty/glslang/glslang/OSDependent/Windows/ossource.cpp
	#	3rdparty/glslang/glslang/OSDependent/Windows/main.cpp
	3rdparty/glslang/glslang/GenericCodeGen/CodeGen.cpp
	3rdparty/glslang/glslang/GenericCodeGen/Link.cpp
	3rdparty/glslang/glslang/MachineIndependent/parseConst.cpp
	3rdparty/glslang/glslang/MachineIndependent/iomapper.cpp
	3rdparty/glslang/glslang/MachineIndependent/Intermediate.cpp
	3rdparty/glslang/glslang/MachineIndependent/limits.cpp
	3rdparty/glslang/glslang/MachineIndependent/Scan.cpp
	3rdparty/glslang/glslang/MachineIndependent/attribute.cpp
	3rdparty/glslang/glslang/MachineIndependent/glslang_tab.cpp
	3rdparty/glslang/glslang/MachineIndependent/SymbolTable.cpp
	3rdparty/glslang/glslang/MachineIndependent/ParseHelper.cpp
	3rdparty/glslang/glslang/MachineIndependent/Constant.cpp
	3rdparty/glslang/glslang/MachineIndependent/InfoSink.cpp
	3rdparty/glslang/glslang/MachineIndependent/RemoveTree.cpp
	3rdparty/glslang/glslang/MachineIndependent/IntermTraverse.cpp
	3rdparty/glslang/glslang/MachineIndependent/Versions.cpp
	3rdparty/glslang/glslang/MachineIndependent/PoolAlloc.cpp
	3rdparty/glslang/glslang/MachineIndependent/intermOut.cpp
	3rdparty/glslang/glslang/MachineIndependent/reflection.cpp
	3rdparty/glslang/glslang/MachineIndependent/Initialize.cpp
	3rdparty/glslang/glslang/MachineIndependent/ParseContextBase.cpp
	3rdparty/glslang/glslang/MachineIndependent/linkValidate.cpp
	3rdparty/glslang/glslang/MachineIndependent/ShaderLang.cpp
	3rdparty/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp
	3rdparty/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp
	3rdparty/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp
	3rdparty/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp
	3rdparty/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp
	3rdparty/glslang/glslang/MachineIndependent/propagateNoContraction.cpp
	)

ADD_EXECUTABLE (shaderc
	../bx/src/amalgamated.cpp

	src/shader.cpp
	src/shader_spirv.cpp
	src/shader_dxbc.cpp
	src/shader_dx9bc.cpp

	tools/shaderc/shaderc.cpp
	tools/shaderc/shaderc_glsl.cpp
	tools/shaderc/shaderc.h
	tools/shaderc/shaderc_hlsl.cpp
	tools/shaderc/shaderc_pssl.cpp
	tools/shaderc/shaderc_spirv.cpp

	${glcpp-library_sources}

	3rdparty/fcpp/cpp1.c
	3rdparty/fcpp/cpp2.c
	3rdparty/fcpp/cpp3.c
	3rdparty/fcpp/cpp4.c
	3rdparty/fcpp/cpp5.c
	3rdparty/fcpp/cpp6.c

	${glslang_SRCS}
	)

SET_TARGET_PROPERTIES ( shaderc PROPERTIES
	COMPILE_FLAGS "-fno-strict-aliasing"
)

TARGET_LINK_LIBRARIES ( shaderc
	pthread
	dl
	)
