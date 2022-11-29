#pragma once
#if __has_include("imgui.h")
#include <mova.h>
#include <imgui.h>

void ImGuiImplMova_Init();
void ImGuiImplMova_NewFrame();
void ImGuiImplMova_Render();
#endif
