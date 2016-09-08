PROJECT ( MESHUP )

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# Needed for UnitTest++
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake )

# Perform the proper linking
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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

INCLUDE_DIRECTORIES (
	${QT_INCLUDE_DIR}/QtOpenGL
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src

	# Required to compile RCPP related code
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bgfx
	)

FIND_PACKAGE (X11 REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

INCLUDE_DIRECTORIES ( 
	3rdparty/glfw/deps
	3rdparty/bx/include
	3rdparty/bgfx/include
	3rdparty/bgfx/examples/common
	3rdparty/bgfx/examples/common/entry
	3rdparty/bgfx/3rdparty
	3rdparty/tinyobjloader
	3rdparty/bgfx/3rdparty/glsl-optimizer/include/
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/
	3rdparty/bgfx/3rdparty/glsl-optimizer/src/mesa/
	${CMAKE_CURRENT_BINARY_DIR}/src
	)

SUBDIRS ( 
	#	tests/ 
	3rdparty/glfw
	3rdparty/bgfx
	)

SET ( protot_SRCS
	src/Renderer.cc
	src/RenderUtils.cc
	src/shaderc_compile.cpp
	src/shaderc_glsl.cpp
	src/shaderc_hlsl.cpp

	src/ModuleManager.cc
	src/TestModule.cpp

	3rdparty/glfw/deps/glad.c
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

ADD_LIBRARY (RuntimeCompiledCpp
	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterfacePerModuleSource.cpp
	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/RuntimeObjectSystem_PlatformPosix.cpp
	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/RuntimeObjectSystem.cpp
	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/RuntimeObjectSystem.cpp

	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/ObjectFactorySystem/ObjectFactorySystem.cpp

	3rdparty/RuntimeCompiledCpp/RuntimeObjectSystem/SimpleSerializer/SimpleSerializer.cpp

	3rdparty/RuntimeCompiledCpp/RuntimeCompiler/BuildTool.cpp  
	3rdparty/RuntimeCompiledCpp/RuntimeCompiler/Compiler_PlatformPosix.cpp
	3rdparty/RuntimeCompiledCpp/RuntimeCompiler/FileChangeNotifier.cpp

	3rdparty/RuntimeCompiledCpp/RuntimeCompiler/SimpleFileWatcher/FileWatcher.cpp
	3rdparty/RuntimeCompiledCpp/RuntimeCompiler/SimpleFileWatcher/FileWatcherLinux.cpp

	3rdparty/RuntimeCompiledCpp/Examples/ConsoleExample/StdioLogSystem.cpp
	
	)

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
	#glew
	glfw
	${glfw_dependencies}
	${OPENGL_LIBRARIES}
	bgfx
	bgfx_aux
	glsl-optimizer
	RuntimeCompiledCpp
)
