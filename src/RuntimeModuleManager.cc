#include "RuntimeModuleManager.h"
#include <GLFW/glfw3.h>

#define _BSD_SOURCE // usleep()
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "RuntimeModule.h"
#include <iostream>
#include <fstream>

#include "Globals.h"

using namespace std;

void RuntimeModuleManager::RegisterModule(const char* name) {
	RuntimeModule* module = new RuntimeModule();
	module->name = name;
	mModules.push_back(module);
}

void RuntimeModuleManager::LoadModule(RuntimeModule* module) {
	struct stat attr;

	bool stat_result = stat(module->name.c_str(), &attr);
	if (glfwGetKey(gWindow, GLFW_KEY_F11) == GLFW_PRESS) {
		std::cerr << "Module " << module->name << " id = " << module->id 
			<< " mtime " << ctime((time_t *) &module->mtime) << std::endl;
	} 

	if ( stat_result == 0 && 
			(module->id != attr.st_ino || module->mtime != attr.st_mtime)
			) {
		std::cout << "Opening module " << module->name << std::endl;
		void *handle = dlopen(module->name.c_str(), RTLD_NOW | RTLD_GLOBAL);
		if (handle) {
			module->handle = handle;
			module->id = attr.st_ino;
			module->mtime = attr.st_mtime;
			const struct module_api *api = (module_api*) dlsym(module->handle, "MODULE_API");
			if (api != NULL) {
				module->api = *api;
				if (module->state == NULL) {
					std::cout << "Initializing module " << module->name << std::endl;
					module->state = module->api.init();
				}
				std::cout << "Reloading module " << module->name << std::endl;
				module->api.reload(module->state);
			} else {
				std::cerr << "Error: could not find API for module " << module->name << std::endl;
				dlclose(module->handle);
				module->handle = NULL;
				module->id = 0;
			}
		} else {
			std::cerr << "Error: could not load module " << module->name << std::endl;
			std::cerr << dlerror() << std::endl;
			module->handle = NULL;
			module->id = 0;
			abort();
		}
	}
}

void RuntimeModuleManager::Update(float dt) {
	if (CheckModulesChanged()) {
		std::cout << "Detected module update. Unloading all modules." << std::endl;

		// We unload in reverse order so that dependencies are cleaned
		// up properly.
		for (int i = mModules.size() - 1; i >= 0; i--) {
			if (mModules[i]->handle) {
				std::cerr << "Unloading module " << mModules[i]->name << std::endl;
				mModules[i]->api.unload(mModules[i]->state);
				dlclose(mModules[i]->handle);
				mModules[i]->handle = 0;
				mModules[i]->id = 0;
			}
		}

		cout << "Writing state to state.lua" << endl;
		// save state
		ofstream current_state("state.lua");
		current_state << (*gSerializer).serialize() << endl;
		current_state.close();

		// We need to sleep to make sure we load the new files
		usleep(200000);
	}

	for (int i = 0; i < mModules.size(); i++) {
		LoadModule(mModules[i]);
		if (mModules[i]->handle) {
			mModules[i]->api.step(mModules[i]->state, dt);
		}
	}
}

bool RuntimeModuleManager::CheckModulesChanged() {
	struct stat attr;

	for (int i = 0; i < mModules.size(); i++) {
		RuntimeModule* module = mModules[i];
		bool stat_result = stat(module->name.c_str(), &attr);

		if ( stat_result == 0 && 
				(module->id != attr.st_ino || module->mtime != attr.st_mtime)
			 ) {
			return true;
		}
	}

	return false;
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

