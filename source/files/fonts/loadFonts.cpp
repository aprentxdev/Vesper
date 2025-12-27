#include "loadFonts.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

// Global pointers to loaded font variants
ImFont* g_RubikRegular = nullptr;
ImFont* g_RubikMedium   = nullptr;
ImFont* g_RubikLarge    = nullptr;

void LoadRubikFont(ImGuiIO& io) {
    std::string fontPath = GetResourcePath("fonts/rubik/Rubik-Medium.ttf");
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.PixelSnapH = true;

    // Latin + Cyrillic ranges
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0400, 0x052F, 0 };

    g_RubikRegular = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &config, ranges);
    g_RubikMedium  = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, &config, ranges);
    g_RubikLarge   = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 28.0f, &config, ranges);

    if (!g_RubikRegular) std::cerr << "Failed to load Rubik font: " << fontPath << std::endl;
}

void LoadFontAwesome(ImGuiIO& io) {
    std::string faPath = GetResourcePath("fonts/fontawesome-free-6.7.2-desktop/otfs/Font Awesome 6 Free-Solid-900.otf");
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;

    static const ImWchar icon_ranges[] = { 0xf000, 0xf8ff, 0 };

    if (!io.Fonts->AddFontFromFileTTF(faPath.c_str(), 16.0f, &config, icon_ranges))
        std::cerr << "Failed to load FontAwesome: " << faPath << std::endl;
}