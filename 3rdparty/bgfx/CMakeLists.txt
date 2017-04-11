PROJECT (BGFX CXX)

FIND_PACKAGE (OpenGL)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#ADD_DEFINITIONS ("BX_PLATFORM_LINUX")
#ADD_DEFINITIONS ("-DBGFX_CONFIG_MULTITHREADED=0")
ADD_DEFINITIONS ("-DBGFX_CONFIG_RENDERER_OPENGL=1")
ADD_DEFINITIONS ("-DENTRY_CONFIG_USE_GLFW=1")

ADD_DEFINITIONS ("-D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS")

INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_SOURCE_DIR}/bx/include
	${CMAKE_CURRENT_SOURCE_DIR}/../bimg/include
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/khronos
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/examples/common
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${OPENGL_INCLUDE_DIR}
	)

SET (BGFX_SOURCES
	../bx/src/amalgamated.cpp
	src/amalgamated.cpp
	)

SET (BGFX_AUX_SOURCES
	examples/common/entry/dbg.cpp
	examples/common/nanovg/nanovg.cpp
	examples/common/nanovg/nanovg_bgfx.cpp
	examples/common/imgui/imgui.cpp
	examples/common/imgui/ocornut_imgui.cpp
	3rdparty/ib-compress/indexbuffercompression.cpp
	3rdparty/ib-compress/indexbufferdecompression.cpp
	3rdparty/stb/stb_image.c
	3rdparty/ocornut-imgui/imgui.cpp
	3rdparty/ocornut-imgui/imgui_draw.cpp
	3rdparty/ocornut-imgui/imgui_demo.cpp
	3rdparty/ocornut-imgui/imgui_node_graph_test.cpp
	)

ADD_LIBRARY ( bgfx SHARED ${BGFX_SOURCES} )
ADD_LIBRARY ( bgfx_aux SHARED ${BGFX_AUX_SOURCES} )

TARGET_LINK_LIBRARIES (bgfx
	${OPENGL_LIBRARIES}
	)
