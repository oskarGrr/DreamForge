#include <imgui.h>

namespace DFE
{

    void themeLight()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.31f, 0.25f, 0.24f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.74f, 0.74f, 0.94f, 1.00f);
        //style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.68f, 0.68f, 0.68f, 0.00f);
        colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.62f, 0.70f, 0.72f, 0.56f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.33f, 0.14f, 0.47f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.97f, 0.31f, 0.13f, 0.81f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.42f, 0.75f, 1.00f, 0.53f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.65f, 0.80f, 0.20f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.40f, 0.62f, 0.80f, 0.15f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.39f, 0.64f, 0.80f, 0.30f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.67f, 0.80f, 0.59f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.48f, 0.53f, 0.67f);
        //style.Colors[ImGuiCol_ComboBg] = ImVec4(0.89f, 0.98f, 1.00f, 0.99f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.48f, 0.47f, 0.47f, 0.71f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.31f, 0.47f, 0.99f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(1.00f, 0.79f, 0.18f, 0.78f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.82f, 1.00f, 0.81f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.72f, 1.00f, 1.00f, 0.86f);
        colors[ImGuiCol_Header] = ImVec4(0.65f, 0.78f, 0.84f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.88f, 0.94f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.68f, 0.74f, 0.80f);//ImVec4(0.46f, 0.84f, 0.90f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.60f, 0.60f, 0.80f, 0.30f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        //style.Colors[ImGuiCol_CloseButton] = ImVec4(0.41f, 0.75f, 0.98f, 0.50f);
        //style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(1.00f, 0.47f, 0.41f, 0.60f);
        //style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(1.00f, 0.16f, 0.00f, 1.00f);
        //style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.99f, 0.54f, 0.43f);
        //style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.82f, 0.92f, 1.00f, 0.90f);
        style.Alpha = 1.0f;
        //style.WindowFillAlphaDefault = 1.0f;
        style.FrameRounding = 4;
        style.IndentSpacing = 12.0f;
        style.GrabRounding = 2;
    }
    
    void themeGreenDark()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                  = {1.00f, 1.00f, 1.00f, 1.00f};
        colors[ImGuiCol_TextDisabled]          = {0.50f, 0.50f, 0.50f, 1.00f};
        colors[ImGuiCol_WindowBg]              = {0.06f, 0.06f, 0.06f, 0.94f};
        colors[ImGuiCol_ChildBg]               = {0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_PopupBg]               = {0.08f, 0.08f, 0.08f, 0.94f};
        colors[ImGuiCol_Border]                = {0.43f, 0.43f, 0.50f, 0.50f};
        colors[ImGuiCol_BorderShadow]          = {0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_FrameBg]               = {0.44f, 0.44f, 0.44f, 0.60f};
        colors[ImGuiCol_FrameBgHovered]        = {0.57f, 0.57f, 0.57f, 0.70f};
        colors[ImGuiCol_FrameBgActive]         = {0.76f, 0.76f, 0.76f, 0.80f};
        colors[ImGuiCol_TitleBg]               = {0.04f, 0.04f, 0.04f, 1.00f};
        colors[ImGuiCol_TitleBgActive]         = {0.16f, 0.16f, 0.16f, 1.00f};
        colors[ImGuiCol_TitleBgCollapsed]      = {0.00f, 0.00f, 0.00f, 0.60f};
        colors[ImGuiCol_MenuBarBg]             = {0.14f, 0.14f, 0.14f, 1.00f};
        colors[ImGuiCol_ScrollbarBg]           = {0.02f, 0.02f, 0.02f, 0.53f};
        colors[ImGuiCol_ScrollbarGrab]         = {0.31f, 0.31f, 0.31f, 1.00f};
        colors[ImGuiCol_ScrollbarGrabHovered]  = {0.41f, 0.41f, 0.41f, 1.00f};
        colors[ImGuiCol_ScrollbarGrabActive]   = {0.51f, 0.51f, 0.51f, 1.00f};
        colors[ImGuiCol_CheckMark]             = {0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_SliderGrab]            = {0.13f, 0.75f, 0.75f, 0.80f};
        colors[ImGuiCol_SliderGrabActive]      = {0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Button]                = {0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_ButtonHovered]         = {0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_ButtonActive]          = {0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Header]                = {0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_HeaderHovered]         = {0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_HeaderActive]          = {0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Separator]             = {0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_SeparatorHovered]      = {0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_SeparatorActive]       = {0.18f, 0.89f, 0.64f, 0.50f};
        colors[ImGuiCol_ResizeGrip]            = {0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_ResizeGripHovered]     = {0.13f, 0.75f, 0.75f, 0.60f};
        colors[ImGuiCol_ResizeGripActive]      = {0.13f, 0.75f, 1.00f, 0.80f};
        colors[ImGuiCol_Tab]                   = {0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_TabHovered]            = {0.13f, 0.75f, 0.75f, 0.80f};
        colors[ImGuiCol_TabActive]             = {0.13f, 0.75f, 0.55f, 0.40f};
        colors[ImGuiCol_TabUnfocused]          = {0.18f, 0.18f, 0.18f, 1.00f};
        colors[ImGuiCol_TabUnfocusedActive]    = {0.36f, 0.36f, 0.36f, 0.54f};
        colors[ImGuiCol_DockingPreview]        = {0.13f, 0.75f, 0.55f, 0.80f};
        colors[ImGuiCol_DockingEmptyBg]        = {0.13f, 0.13f, 0.13f, 0.80f};
        colors[ImGuiCol_PlotLines]             = {0.61f, 0.61f, 0.61f, 1.00f};
        colors[ImGuiCol_PlotLinesHovered]      = {1.00f, 0.43f, 0.35f, 1.00f};
        colors[ImGuiCol_PlotHistogram]         = {0.90f, 0.70f, 0.00f, 1.00f};
        colors[ImGuiCol_PlotHistogramHovered]  = {1.00f, 0.60f, 0.00f, 1.00f};
        colors[ImGuiCol_TableHeaderBg]         = {0.19f, 0.19f, 0.20f, 1.00f};
        colors[ImGuiCol_TableBorderStrong]     = {0.31f, 0.31f, 0.35f, 1.00f};
        colors[ImGuiCol_TableBorderLight]      = {0.23f, 0.23f, 0.25f, 1.00f};
        colors[ImGuiCol_TableRowBg]            = {0.00f, 0.00f, 0.00f, 0.00f};
        colors[ImGuiCol_TableRowBgAlt]         = {1.00f, 1.00f, 1.00f, 0.07f};
        colors[ImGuiCol_TextSelectedBg]        = {0.26f, 0.59f, 0.98f, 0.35f};
        colors[ImGuiCol_DragDropTarget]        = {1.00f, 1.00f, 0.00f, 0.90f};
        colors[ImGuiCol_NavHighlight]          = {0.26f, 0.59f, 0.98f, 1.00f};
        colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
        colors[ImGuiCol_NavWindowingDimBg]     = {0.80f, 0.80f, 0.80f, 0.20f};
        colors[ImGuiCol_ModalWindowDimBg]      = {0.80f, 0.80f, 0.80f, 0.35f};
    }
    
    void themeSuperDark()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding                     = ImVec2(8.00f, 8.00f);
        style.FramePadding                      = ImVec2(5.00f, 2.00f);
        style.CellPadding                       = ImVec2(6.00f, 6.00f);
        style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
        style.IndentSpacing                     = 25;
        style.ScrollbarSize                     = 15;
        style.GrabMinSize                       = 10;
        style.WindowBorderSize                  = 1;
        style.ChildBorderSize                   = 1;
        style.PopupBorderSize                   = 1;
        style.FrameBorderSize                   = 1;
        style.TabBorderSize                     = 1;
        style.WindowRounding                    = 7;
        style.ChildRounding                     = 4;
        style.FrameRounding                     = 3;
        style.PopupRounding                     = 4;
        style.ScrollbarRounding                 = 9;
        style.GrabRounding                      = 3;
        style.LogSliderDeadzone                 = 4;
        style.TabRounding                       = 4;
    }
    
    void themeDarkRedOrange()
    {
        themeSuperDark();
        ImVec4* colors = ImGui::GetStyle().Colors;
        ImGuiStyle& style = ImGui::GetStyle();
        colors[ImGuiCol_Header]           = {.332f, .184f, .184f, .520f};
        colors[ImGuiCol_HeaderHovered]    = {.904f, .633f, .633f, .231f};
        colors[ImGuiCol_Text]             = {.973f, .932f, .933f, .935f};
        colors[ImGuiCol_CheckMark]        = {.818f, .539f, .245f, .9f};
        colors[ImGuiCol_Border].w = 0.0f;
        colors[ImGuiCol_SliderGrab]       = {.99f, .99f, .2, .7};//= colors[ImGuiCol_CheckMark];
        colors[ImGuiCol_SliderGrab].w = .78f;
        colors[ImGuiCol_SliderGrabActive] = {.727f, .727f, .727f, .540f};
        colors[ImGuiCol_Separator]        = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_Separator].w = .67f;
        colors[ImGuiCol_Tab]              = {.332f, .184f, .184f, .520f};
        colors[ImGuiCol_TabActive]        = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive].w = .5f;
        style.WindowRounding = 3;
    }

}//namespace DFE