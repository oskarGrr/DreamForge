#include <memory>
#include <iostream>

#include "DreamForge.hpp"
#include "GUITheme.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

class EditorApp : public DF::DreamForgeApp
{
public:
    EditorApp()=default;
    ~EditorApp()=default;
};

int main(int argumentCount, char** argumentVector)
{
    EditorApp app;

    //DFE::themeLight();

    DF::Logger::get().stdoutInfo("hello from the editor app\n");
    app.run();
}