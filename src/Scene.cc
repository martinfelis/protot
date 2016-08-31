#include "Scene.h"

// Runtime Compiled Cpp
#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObjectFactorySystem.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterface.h"
#include "RuntimeCompiledCpp/RuntimeCompiler/AUArray.h"

#include "RuntimeCompiledCpp/RuntimeCompiler/BuildTool.h"
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

using namespace std;

Scene::Scene() :
	mCompilerLogger(nullptr),
	mRuntimeObjectSystem(nullptr),
	mUpdateable(nullptr) {
}

Scene::~Scene() {
	if (mRuntimeObjectSystem != nullptr) {
		mRuntimeObjectSystem->CleanObjectFiles();
	}

	if (mRuntimeObjectSystem && mRuntimeObjectSystem->GetObjectFactorySystem()) {
		mRuntimeObjectSystem->GetObjectFactorySystem()->RemoveListener(this);

		// delete object via correct interface
		IObject* obj = mRuntimeObjectSystem->GetObjectFactorySystem()->GetObject( mObjectId );
		delete obj;
	}

	delete mRuntimeObjectSystem;
	delete mCompilerLogger;
}

bool Scene::init() {
	mRuntimeObjectSystem = new RuntimeObjectSystem;
	mCompilerLogger = new StdioLogSystem();

	if (!mRuntimeObjectSystem->Initialise(mCompilerLogger, 0)) {
		mRuntimeObjectSystem = nullptr;
		return false;
	}

	mRuntimeObjectSystem->GetObjectFactorySystem()->AddListener(this);

	// construct an object
	IObjectConstructor* constructor = mRuntimeObjectSystem->GetObjectFactorySystem()->GetConstructor("SceneObject");
	if (constructor) {
		IObject* obj = constructor->Construct();
		obj->GetInterface(&mUpdateable);
		
		if (nullptr == mUpdateable) {
			delete obj;
			mCompilerLogger->LogError("Error - no updateable interface found!\n");
			return false;
		}

		mObjectId = obj->GetObjectId();
	}

	return true;
}

void Scene::OnConstructorsAdded() {
	if (mUpdateable) {
		IObject* obj = mRuntimeObjectSystem->GetObjectFactorySystem()->GetObject(mObjectId);
		obj->GetInterface(&mUpdateable);
		
		if (nullptr == mUpdateable) {
			delete obj;
			mCompilerLogger->LogError("Error - no updateable interface found!\n");
		}
	}
}

void Scene::update() {
	if (mRuntimeObjectSystem->GetIsCompiledComplete()) {
		mRuntimeObjectSystem->LoadCompiledModule();
	}

	if (!mRuntimeObjectSystem->GetIsCompiling()) {
		static int num_updates = 0;
		std::cout << "Main loop. num_updates = " << num_updates << "\n";

		const float delta_time = 1.0f;
		mRuntimeObjectSystem->GetFileChangeNotifier()->Update(delta_time);
		mUpdateable->Update(delta_time);
		usleep(1000 * 1000);
	}
}
