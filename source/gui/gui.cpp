#include "gui.h"

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_WindowBg]        = ImVec4(0.110f, 0.110f, 0.125f, 1.000f);
    colors[ImGuiCol_ChildBg]         = ImVec4(0.110f, 0.110f, 0.125f, 1.000f);
    colors[ImGuiCol_PopupBg]         = ImVec4(0.130f, 0.130f, 0.150f, 0.980f);
    colors[ImGuiCol_TitleBg]         = colors[ImGuiCol_WindowBg];
    colors[ImGuiCol_TitleBgActive]   = colors[ImGuiCol_WindowBg];
    colors[ImGuiCol_Button]          = ImVec4(0.20f, 0.42f, 0.86f, 0.80f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.26f, 0.50f, 0.94f, 1.00f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.15f, 0.38f, 0.82f, 1.00f);
    colors[ImGuiCol_Header]          = ImVec4(0.22f, 0.45f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.26f, 0.50f, 0.95f, 0.70f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.28f, 0.52f, 0.96f, 0.90f);
    colors[ImGuiCol_PlotHistogram]   = ImVec4(0.26f, 0.50f, 0.94f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(0.160f, 0.160f, 0.180f, 1.000f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.200f, 0.200f, 0.230f, 1.000f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.22f, 0.45f, 0.90f, 0.35f);
    colors[ImGuiCol_Text]            = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]     = ImVec4(0.15f, 0.15f, 0.17f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]   = ImVec4(0.35f, 0.35f, 0.40f, 0.90f);

    style.WindowRounding = 0.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(5, 5);
    style.ItemSpacing = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.ScrollbarSize = 15.0f;
}

void SetupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    io.IniFilename = NULL;
    LoadFontAwesome(io);
    LoadRubikFont(io);

    SetupImGuiStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}