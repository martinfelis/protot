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
		gLog ("Module %s id = %d mtime %d", module->name.c_str(), module->id, module->mtime);
	} 

	if ( stat_result == 0 && 
			(module->id != attr.st_ino || module->mtime != attr.st_mtime)
			) {
		gLog ("Loading module %s (size %d)", module->name.c_str(), attr.st_size);
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
					gLog("Initializing module %s", module->name.c_str());
					module->state = module->api.init();
				}
				gLog("Reloading module %s", module->name.c_str());
				module->api.reload(module->state, gReadSerializer);
			} else {
				gLog("Loading module failed: Could not find API for module %s", module->name.c_str());
				dlclose(module->handle);
				module->handle = NULL;
				module->id = 0;
			}
		} else {
			gLog ("Loading module failed: could not load shared library for module %s: %s", module->name.c_str(), dlerror());
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
			gLog ("Unloading shared library %s", mModules[i]->name.c_str());
			int unload_result = dlclose(mModules[i]->handle);
			if (unload_result != 0) {
				gLog ("Unload failed (code %d): %s", unload_result, dlerror());
			}
			mModules[i]->handle = 0;
			mModules[i]->id = 0;
			gLog("Unloading module %s complete", mModules[i]->name.c_str());
		}
	}

	gLog ("Writting state to file %s", state_file);
	gWriteSerializer->Close();
}

void RuntimeModuleManager::LoadModules() {
	gLog ("Reading state from file %s", state_file);
	gReadSerializer->Open(state_file);
	for (int i = 0; i < mModules.size(); i++) {
		LoadModule(mModules[i]);
	}
	gReadSerializer->Close();
}
