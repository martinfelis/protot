#pragma once

#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterface.h"

struct IUpdateable;
struct IRuntimeObjectSystem;

struct Scene : IObjectFactoryListener {
	ICompilerLogger* mCompilerLogger;
	IRuntimeObjectSystem* mRuntimeObjectSystem;

	IUpdateable* mUpdateable;
	ObjectId mObjectId;

	Scene();
	virtual ~Scene();

	bool init();
	void OnConstructorsAdded();
	void update ();
};
