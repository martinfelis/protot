INCLUDE_DIRECTORIES (
		${CMAKE_CURRENT_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/3rdparty/rbdl/include
		${PROJECT_SOURCE_DIR}/3rdparty/rbdl/
		${PROJECT_BINARY_DIR}/3rdparty/rbdl/include
	)

ADD_LIBRARY (RenderModule SHARED 
	RenderModule.cc
	RenderUtils.cc
	)

ADD_LIBRARY (TestModule SHARED
	TestModule.cc
	)

ADD_LIBRARY (CharacterModule SHARED
	CharacterModule.cc
	)

TARGET_LINK_LIBRARIES ( CharacterModule
	RenderModule
	)

TARGET_LINK_LIBRARIES ( TestModule
	CharacterModule
	RenderModule
	)
