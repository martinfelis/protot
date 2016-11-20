#pragma once

struct Renderer;
extern Renderer* gRenderer;

struct GLFWwindow;
extern GLFWwindow* gWindow;

struct RuntimeModuleManager;
extern RuntimeModuleManager* gModuleManager;

#include "Serializer.h"
extern LuaTable* gSerializer;
