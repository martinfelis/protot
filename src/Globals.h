#pragma once

#include "math_types.h"
#include "Utils.h"

struct Timer;
extern Timer* gTimer;

struct Renderer;
extern Renderer* gRenderer;

struct GLFWwindow;
extern GLFWwindow* gWindow;

struct RuntimeModuleManager;
extern RuntimeModuleManager* gModuleManager;

struct WriteSerializer;
extern WriteSerializer* gWriteSerializer;

struct ReadSerializer;
extern ReadSerializer* gReadSerializer;

struct GuiInputState;
extern GuiInputState* gGuiInputState;

struct FileModificationObserver;
extern FileModificationObserver* gFileModificationObserver;
