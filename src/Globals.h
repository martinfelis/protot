#pragma once

#include "math_types.h"

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
