PROJECT ( MESHUP )

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# Needed for UnitTest++
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake )

# Perform the proper linking
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# For YouCompleteMe
SET (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable proper C++11 flags
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# enable strings in GDB
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")

# enable LuaModel addon for RBDL
SET (RBDL_BUILD_ADDON_LUAMODEL ON)
SET (RBDL_USE_SIMPLE_MATH ON)

INCLUDE_DIRECTORIES (
	${QT_INCLUDE_DIR}/QtOpenGL
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src

	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bgfx

	# RBDL and its LuaModel addon
	${CMAKE_CURRENT_BINARY_DIR}/3rdparty/rbdl/
	${CMAKE_CURRENT_BINARY_DIR}/3rdparty/rbdl/include
	)

FIND_PACKAGE (X11 REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

INCLUDE_DIRECTORIES ( 
	3rdparty/glfw/deps
	3rdparty/glfw/include
	3rdparty/bx/include
	3rdparty/bgfx/include
	3rdparty/bgfx/examples/common
	3rdparty/bgfx/examples/common/entry
	3rdparty/bgfx/3rdparty
	3rdparty/tinyobjloader
	3rdparty/bgfx/3rdparty/glsl-optimizer/include/
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/mesa/
	3rdparty/imgui_dock/
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bgfx/3rdparty/ocornut-imgui
	${CMAKE_CURRENT_BINARY_DIR}/src
	)

SUBDIRS ( 
	src/modules
	tests/

	3rdparty/glfw
	3rdparty/bimg
	3rdparty/bgfx
	3rdparty/rbdl
	3rdparty/googletest
	)

SET ( protot_SRCS
	src/RuntimeModuleManager.cc
	src/Utils.cc

	3rdparty/glfw/deps/glad.c
	3rdparty/imgui_dock/imgui_dock.cpp
	)

SET (PROTOT_SOURCE_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

CONFIGURE_FILE (
	"${CMAKE_CURRENT_SOURCE_DIR}/src/protot_config.h.cmake" 
	"${CMAKE_CURRENT_BINARY_DIR}/src/protot_config.h"
	)

ADD_EXECUTABLE (protot src/main.cc ${protot_SRCS})

file(GLOB glcpp-library_sources 
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/mesa/main/*.c
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/mesa/program/*.c
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/*.c 
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/glsl/*.cpp 
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/glsl/*.c
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/util/*.c
	)

list(REMOVE_ITEM glcpp-library_sources ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bgfx/3rdparty/glsl-optimizer/src/glsl/main.cpp)

ADD_LIBRARY (glsl-optimizer
	${glcpp-library_sources}

	3rdparty/bgfx/3rdparty/fcpp/cpp1.c
	3rdparty/bgfx/3rdparty/fcpp/cpp2.c
	3rdparty/bgfx/3rdparty/fcpp/cpp3.c
	3rdparty/bgfx/3rdparty/fcpp/cpp4.c
	3rdparty/bgfx/3rdparty/fcpp/cpp5.c
	3rdparty/bgfx/3rdparty/fcpp/cpp6.c
	)

SET_TARGET_PROPERTIES ( ${PROJECT_EXECUTABLES} PROPERTIES
	LINKER_LANGUAGE CXX
	INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" 
)

SET_TARGET_PROPERTIES ( glsl-optimizer PROPERTIES
	COMPILE_FLAGS "-fno-strict-aliasing"
)

MESSAGE (STATUS "glfw libraries: ${X11_X11_LIB}")

SET (glfw_dependencies
	${X11_X11_LIB}
	${X11_Xrandr_LIB}
	${X11_Xinerama_LIB}
	${X11_Xinput_LIB}
	${X11_Xcursor_LIB}
	${X11_Xxf86vm_LIB}
	${CMAKE_DL_LIBS}
	${CMAKE_THREAD_LIBS_INIT}
	)

TARGET_LINK_LIBRARIES ( protot
	glfw
	${glfw_dependencies}
	${OPENGL_LIBRARIES}
	bimg
	bgfx
	bgfx_aux
	glsl-optimizer
	rbdl
	rbdl_luamodel
)
