#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GuiLoop.h"
#include <iostream>

#include "loadFonts.h"

void SetupImGuiStyle();
void SetupImGui(GLFWwindow* window);