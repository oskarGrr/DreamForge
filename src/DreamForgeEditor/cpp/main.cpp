#include <memory>
#include <iostream>

#include "DreamForge.hpp"

#include "GUITheme.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

//namespace Dream Forge Editor
namespace DFE
{

class EditorApp : public DF::ApplicationBase
{
public:
    EditorApp()
    {
        ImGui::SetCurrentContext(getImGuiContext());
        DFE::themeDarkRedOrange();
    }

    void imguiDraw() const override
    {
        
    }

    ~EditorApp()=default;
};

}

int main(int argumentCount, char** argumentVector)
{
    DFE::EditorApp app;
    DF::Logger::get().stdoutInfo("hello from the editor app\n");

    app.run();

    return EXIT_SUCCESS;
}