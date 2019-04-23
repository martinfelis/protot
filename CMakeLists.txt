PROJECT ( Protot )

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

# Enable CCache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

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

message(STATUS "Using compiler flags ${CMAKE_CXX_FLAGS}")

add_definitions("-DIMGUI_IMPL_OPENGL_LOADER_GLAD")

# setup of libccd variables so that FCL can find it
SET (CCD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libccd/src)
SET (CCD_LIBRARY ccd)
SET (BUILD_SHARED_LIBS TRUE)
SET (FCL_BUILD_TESTS FALSE)
SET (BUILD_TESTING OFF)
SET (GLFW_BUILD_DOCS OFF)
SET (GLFW_BUILD_EXAMPLES OFF)

INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src

	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	)

FIND_PACKAGE (X11 REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

INCLUDE_DIRECTORIES ( 
	3rdparty/glfw/deps
	3rdparty/glfw/include
	3rdparty/tinyobjloader
	3rdparty/imgui_dock_lumix
	${CMAKE_CURRENT_BINARY_DIR}/src

	3rdparty/imgui
	3rdparty/imgui/examples/
	)

SUBDIRS ( 
	src/modules
	tests/

	3rdparty/glfw
	3rdparty/googletest
	)

SET (GLFW_BUILD_DOCS OFF)
SET (GLFW_BUILD_EXAMPLES OFF)
SET (GLFW_BUILD_TESTS OFF)

SET ( protot_SRCS
	src/RuntimeModuleManager.cc
	src/Utils.cc

	3rdparty/glfw/deps/glad.cc
	3rdparty/imgui/imgui.cpp
	3rdparty/imgui/imgui_draw.cpp
	3rdparty/imgui/imgui_widgets.cpp
	3rdparty/imgui/imgui_demo.cpp
	3rdparty/imgui/examples/imgui_impl_glfw.cpp
	3rdparty/imgui/examples/imgui_impl_opengl3.cpp

#	3rdparty/imgui_dock_lumix/imgui_dock.cpp
	)

SET (PROTOT_SOURCE_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

CONFIGURE_FILE (
	"${CMAKE_CURRENT_SOURCE_DIR}/src/protot_config.h.cmake" 
	"${CMAKE_CURRENT_BINARY_DIR}/src/protot_config.h"
	)

ADD_EXECUTABLE (protot src/main.cc ${protot_SRCS})

SET_TARGET_PROPERTIES ( ${PROJECT_EXECUTABLES} PROPERTIES
	LINKER_LANGUAGE CXX
	INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" 
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
)
