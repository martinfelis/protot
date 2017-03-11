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
#include "Serializer.h"

using namespace std;
const char* state_file = "state.ser";

void RuntimeModuleManager::RegisterModule(const char* name) {
	RuntimeModule* module = new RuntimeModule();
	module->name = name;
	mModules.push_back(module);
}

void RuntimeModuleManager::UnregisterModules() {
	UnloadModules();

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
		std::cout << "Opening module " << module->name 
			<< " (size = " << attr.st_size << ")" << std::endl;
		void *handle = dlopen(module->name.c_str(), RTLD_NOW | RTLD_GLOBAL);
		if (handle) {
			module->handle = handle;
			module->id = attr.st_ino;
			module->mtime = attr.st_mtime;
			module->mtimensec = attr.st_mtim.tv_nsec;
			const struct module_api *api = (module_api*) dlsym(module->handle, "MODULE_API");
			if (api != NULL) {
				module->api = *api;
				if (module->state == NULL) {
					std::cout << "Initializing module " << module->name << std::endl;
					module->state = module->api.init();
				}
				std::cout << "Reloading module " << module->name << std::endl;
				module->api.reload(module->state, gReadSerializer);
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
	for (int i = mModules.size() - 1; i >= 0; i--) {
		if (mModules[i]->handle) {
			mModules[i]->api.step(mModules[i]->state, dt);
		}
	}
}

bool RuntimeModuleManager::CheckModulesChanged() {
	struct stat attr;

	double current_time = gGetTimeSinceStart();
	mNumUpdatesSinceLastModuleChange++;

	for (int i = 0; i < mModules.size(); i++) {
		RuntimeModule* module = mModules[i];
		bool stat_result = stat(module->name.c_str(), &attr);

		if (stat_result != 0) {
			gLog ("Error: could not stat module %s", module->name.c_str());
			abort();
		}
		
		if (    module->id != attr.st_ino 
				 || module->mtime != attr.st_mtime
				 || module->fsize != attr.st_size
				 || module->fsize == 0 
			 ) {
			module->id = attr.st_ino;
			module->mtime = attr.st_mtime;
			module->mtimensec = attr.st_mtim.tv_nsec;
			module->fsize = attr.st_size;
			mNumUpdatesSinceLastModuleChange = 0;

			gLog ("Detected file change of %s: new size %d",
					module->name.c_str(), attr.st_size);
		}
	}

	// We have to delay the actual reload trigger to make
	// sure all writes to the dynamic libraries are complete.
	if (mNumUpdatesSinceLastModuleChange == 5) {
		gLog ("Triggering reload");

		return true;
	}


	return false;
}

void RuntimeModuleManager::UnloadModules() {
	gWriteSerializer->Open(state_file);

	for (int i = mModules.size() - 1; i >= 0 ; i--) {
		if (mModules[i]->handle) {
			gLog("Unloading module %s", mModules[i]->name.c_str());
			mModules[i]->api.unload(mModules[i]->state, gWriteSerializer);
			mModules[i]->state = nullptr;
			dlclose(mModules[i]->handle);
			mModules[i]->handle = 0;
			mModules[i]->id = 0;
		}
	}

	std::cout << "Writing state to file " << state_file << std::endl;
	gWriteSerializer->Close();
}

void RuntimeModuleManager::LoadModules() {
	std::cout << "Reading state from file " << state_file << std::endl;
	gReadSerializer->Open(state_file);
	for (int i = 0; i < mModules.size(); i++) {
		LoadModule(mModules[i]);
	}
	gReadSerializer->Close();
}
