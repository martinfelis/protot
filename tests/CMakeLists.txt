set (GOOGLETEST_DIR
	${CMAKE_SOURCE_DIR}/3rdparty/googletest/googletest
	)

include_directories (
	${GOOGLETEST_DIR}/include
	${CMAKE_SOURCE_DIR}/3rdparty/bx/include
	${CMAKE_SOURCE_DIR}/3rdparty/bx/3rdparty
	)

set (TEST_SRCS
	RenderModuleTests.cc
	${GOOGLETEST_DIR}/src/gtest_main.cc
	${CMAKE_SOURCE_DIR}/3rdparty/bx/src/amalgamated.cpp
	)

add_executable ( runtests ${TEST_SRCS} )

target_link_libraries (runtests
	${CMAKE_DL_LIBS}
	gtest
)
