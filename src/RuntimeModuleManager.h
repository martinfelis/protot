#pragma once

#include <string>
#include <vector>

struct RuntimeModule;

struct RuntimeModuleManager {
	std::vector<RuntimeModule*> mModules;

	void RegisterModule(const char* name);
	void LoadModule(RuntimeModule* module);
	bool CheckModulesChanged();
	void UnloadModules();
	void Update(float dt);
};
