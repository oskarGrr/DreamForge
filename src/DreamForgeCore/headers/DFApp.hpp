#pragma once
#include "Window.hpp"
#include "df_export.hpp"
#include "VulkanRenderer.hpp"
#include "ErrorHandling.hpp"
#include <chrono>

struct ImGuiContext;

namespace DF
{

class DF_DLL_API DreamForgeApp
{
public:
    
    DreamForgeApp();
    virtual ~DreamForgeApp()=default;
    
    //optionally override this to draw a debug menu with imgui.
    virtual void imguiDraw() const {}
    
    void run();
    void processWindowEvents();
    glm::vec<2, double> getMousePos();

    struct guiContext
    {
        guiContext(NonOwningPtr<GLFWwindow> wnd);
        ~guiContext();
        ImGuiContext* context {nullptr};
    };
    NonOwningPtr<ImGuiContext> getImGuiContext() const {return mImGuiContex.context;}

private:

    //begin and end of main engine loop
    std::chrono::steady_clock::time_point startOfLoop();
    double endOfLoop(std::chrono::steady_clock::time_point const frameStartTime);

    bool mIsAppRunning {false};
    double mFrameTime{0.0};
    Window mWindow;
    std::string_view const mTitle {"Dream Forge"};
    VulkanRenderer mRenderer {mWindow};
    guiContext mImGuiContex {mWindow.getRawWindow()};

public:
    DreamForgeApp(DreamForgeApp const&)=delete;
    DreamForgeApp(DreamForgeApp&&)=delete;
    DreamForgeApp& operator=(DreamForgeApp const&)=delete;
    DreamForgeApp& operator=(DreamForgeApp&&)=delete;
};

}
