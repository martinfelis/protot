#include "RuntimeModuleManager.h"

#define _BSD_SOURCE // usleep()
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "RuntimeModule.h"
#include <iostream>

struct RuntimeModule {
	std::string name ="";
	void *handle = nullptr;
	ino_t id = 0;
	void *data = nullptr;

	struct module_api api;
	struct module_state *state = nullptr;
};

void RuntimeModuleManager::RegisterModule(const char* name) {
	RuntimeModule* module = new RuntimeModule();
	module->name = name;
	mModules.push_back(module);
}

void RuntimeModuleManager::LoadModule(RuntimeModule* module) {
	struct stat attr;
	if ((stat(module->name.c_str(), &attr) == 0) && (module->id != attr.st_ino)) {
		std::cerr << "Detected update of module " << module->name << std::endl;
		if (module->handle) {
			std::cerr << "Unloading module " << module->name << std::endl;
			module->api.unload(module->state);
			dlclose(module->handle);
		}
		std::cerr << "Opening module " << module->name << std::endl;
		void *handle = dlopen(module->name.c_str(), RTLD_NOW);
		if (handle) {
			module->handle = handle;
			module->id = attr.st_ino;
			std::cerr << "Loading API symbol" << std::endl;
			const struct module_api *api = (module_api*) dlsym(module->handle, "MODULE_API");
			if (api != NULL) {
				module->api = *api;
				if (module->state == NULL) {
					std::cerr << "Initializing module" << std::endl;
					module->state = module->api.init();
				}
				std::cerr << "Reloading module" << std::endl;
				module->api.reload(module->state);
			} else {
				dlclose(module->handle);
				module->handle = NULL;
				module->id = 0;
			}
		} else {
			module->handle = NULL;
			module->id = 0;
		}
	}
}

void RuntimeModuleManager::Update(float dt) {
	for (int i = 0; i < mModules.size(); i++) {
		LoadModule(mModules[i]);
		if (mModules[i]->handle) {
			mModules[i]->api.step(mModules[i]->state);
		}
	}
}

void RuntimeModuleManager::UnloadModules() {
	for (int i = 0; i < mModules.size(); i++) {
		if (mModules[i]->handle) {
			mModules[i]->api.finalize(mModules[i]->state);
			mModules[i]->state = nullptr;
			dlclose(mModules[i]->handle);
			mModules[i]->handle = 0;
			mModules[i]->id = 0;
			delete mModules[i];
		}
	}
}


