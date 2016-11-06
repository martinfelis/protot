#pragma once

#include <string>
#include <vector>

#include "RuntimeModule.h"

struct RuntimeModule;

struct RuntimeModule {
	std::string name = "";
	void *handle = nullptr;
	ino_t id = 0;
	void *data = nullptr;
	int mtime = 0;

	struct module_api api;
	struct module_state *state = nullptr;
};

struct RuntimeModuleManager {
	std::vector<RuntimeModule*> mModules;

	void RegisterModule(const char* name);
	void LoadModule(RuntimeModule* module);
	bool CheckModulesChanged();
	void UnloadModules();
	void Update(float dt);
};
