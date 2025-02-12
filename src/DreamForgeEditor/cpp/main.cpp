#include <memory>
#include <iostream>

#include "DreamForge.hpp"
#include "GUITheme.hpp"
#include "HelpfulTypeAliases.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

class EditorApp : public DF::DreamForgeApp
{
public:
    EditorApp()
    {
        /*ImGui::SetCurrentContext(getImGuiContext());
        DFE::themeDarkRedOrange();*/
    }

    ~EditorApp()=default;
};

int main(int argumentCount, char** argumentVector)
{
    EditorApp app;

    DF::Logger::get().stdoutInfo("hello from the editor app\n");
    app.run();
}