#include "ModuleManager.h"

#include "protot_config.h"

// Runtime Compiled Cpp
#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterface.h"

#include "RuntimeCompiledCpp/RuntimeCompiler/ICompilerLogger.h"
#include "RuntimeCompiledCpp/RuntimeCompiler/FileChangeNotifier.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectFactorySystem/ObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/RuntimeObjectSystem.h"

#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObject.h"

#include "rcpp/StdioLogSystem.h"
#include "rcpp/IUpdateable.h"

// for sleep
#include <iostream>
#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

ModuleManager::ModuleManager() :
	mCompilerLogger(nullptr),
	mRuntimeObjectSystem(nullptr)
	{
}

ModuleManager::~ModuleManager() {
	if (mRuntimeObjectSystem != nullptr) {
		mRuntimeObjectSystem->CleanObjectFiles();
	}

	if (mRuntimeObjectSystem && mRuntimeObjectSystem->GetObjectFactorySystem()) {
		mRuntimeObjectSystem->GetObjectFactorySystem()->RemoveListener(this);

		for (unsigned int i = 0; i < mModuleIds.size(); i++) {
			// delete object via correct interface
			IObject* obj = mRuntimeObjectSystem->GetObjectFactorySystem()->GetObject( mModuleIds[i] );
			delete obj;
		}
	}

	delete mRuntimeObjectSystem;
	delete mCompilerLogger;
}

bool ModuleManager::init() {
	mRuntimeObjectSystem = new RuntimeObjectSystem;
	mCompilerLogger = new StdioLogSystem();

	if (!mRuntimeObjectSystem->Initialise(mCompilerLogger, 0)) {
		mRuntimeObjectSystem = nullptr;
		return false;
	}

	mRuntimeObjectSystem->GetObjectFactorySystem()->AddListener(this);

	// Setup of the compiler settings
	mRuntimeObjectSystem->SetAdditionalCompileOptions("--std=c++11");

	// add include paths
	string protot_source_root_path = PROTOT_SOURCE_ROOT_PATH;
	mRuntimeObjectSystem->AddIncludeDir (
			string(protot_source_root_path + "/3rdparty/bgfx/include"
				).c_str());
	mRuntimeObjectSystem->AddIncludeDir (
			string(protot_source_root_path + "/3rdparty/bgfx/"
				).c_str());
	mRuntimeObjectSystem->AddIncludeDir (
			string(protot_source_root_path + "/3rdparty/bgfx/3rdparty"
				).c_str());
	mRuntimeObjectSystem->AddIncludeDir (
			string(protot_source_root_path + "/3rdparty/bgfx/examples/common"
				).c_str());


	return true;
}

void ModuleManager::OnConstructorsAdded() {
	for (unsigned int i = 0; i < mModules.size(); i++) {
		IUpdateable* module = mModules[i];

		if (!module)
			continue;

		IObject* obj = mRuntimeObjectSystem->GetObjectFactorySystem()->GetObject(mModuleIds[i]);
		obj->GetInterface(&mModules[i]);
		
		if (nullptr == module) {
			delete obj;
			mCompilerLogger->LogError("Error - no updateable interface found!\n");
		}
	}
}

void ModuleManager::update(float dt) {
	if (mRuntimeObjectSystem->GetIsCompiledComplete()) {
		mRuntimeObjectSystem->LoadCompiledModule();
	}

	if (!mRuntimeObjectSystem->GetIsCompiling()) {
		mRuntimeObjectSystem->GetFileChangeNotifier()->Update(dt);

		for (unsigned int i = 0; i < mModules.size(); i++) {
			mModules[i]->Update(dt);
		}
	}
}

bool ModuleManager::RegisterModule(const char* name) {
	cout << "Registering Module: " << name << endl;

	// construct an object
	IObjectConstructor* constructor = mRuntimeObjectSystem->GetObjectFactorySystem()->GetConstructor(name);
	if (constructor) {
		IObject* obj = constructor->Construct();
		IUpdateable** module = new IUpdateable*[1];
		obj->GetInterface(module);
		
		if (nullptr == module) {
			delete obj;
			mCompilerLogger->LogError("Error - no updateable interface found!\n");
			return false;
		}

		mModules.push_back(*module);
		mModuleIds.push_back(obj->GetObjectId());
	}

	return true;
}
