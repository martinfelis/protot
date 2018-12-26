INCLUDE_DIRECTORIES (
		${CMAKE_CURRENT_SOURCE_DIR}
	)

ADD_LIBRARY (RenderModule SHARED 
	RenderModule.cc
	RenderUtils.cc
	gltf_implementation.cc
	)

ADD_LIBRARY (TestModule SHARED
	TestModule.cc
	)

TARGET_LINK_LIBRARIES ( TestModule
	RenderModule
	)
