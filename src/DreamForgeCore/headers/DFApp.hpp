#pragma once

#include "Window.hpp"
#include "df_export.hpp"
#include "VulkanRenderer.hpp"
#include <chrono>

namespace DF
{

class DF_DLL_API DreamForgeApp
{
public:
    
    DreamForgeApp();
    virtual ~DreamForgeApp()=default;
    
    DreamForgeApp(DreamForgeApp const&)=delete;
    DreamForgeApp(DreamForgeApp&&)=delete;
    DreamForgeApp& operator=(DreamForgeApp const&)=delete;
    DreamForgeApp& operator=(DreamForgeApp&&)=delete;
    
    void run();

    void processWindowEvents();

    glm::vec<2, double> getMousePos();

private:

    //begin and end of main engine loop
    std::chrono::steady_clock::time_point startOfLoop(double dt);
    double endOfLoop(std::chrono::steady_clock::time_point const frameStartTime);

    bool mIsAppRunning;
    Window mWindow;
    VulkanRenderer mRenderer {mWindow};
};

}