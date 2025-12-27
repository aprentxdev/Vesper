#pragma once

#include "imgui.h"
#include "files.h"
#include <string>

extern ImFont* g_RubikRegular;
extern ImFont* g_RubikMedium;
extern ImFont* g_RubikLarge;

void LoadRubikFont(ImGuiIO& io);
void LoadFontAwesome(ImGuiIO& io);