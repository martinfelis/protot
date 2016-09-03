#pragma once

#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterface.h"

struct IUpdateable;
struct IRuntimeObjectSystem;

struct ModuleManager : IObjectFactoryListener {
	ICompilerLogger* mCompilerLogger;
	IRuntimeObjectSystem* mRuntimeObjectSystem;

	std::vector<IUpdateable*> mModules;
	std::vector<ObjectId> mModuleIds;

	ModuleManager();
	virtual ~ModuleManager();

	bool init();
	void update ();

	void OnConstructorsAdded();
	bool RegisterModule(const char* name);

};
