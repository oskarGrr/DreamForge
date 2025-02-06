#pragma once

#include "Window.hpp"
#include "df_export.hpp"
#include "VulkanRenderer.hpp"

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

    glm::vec<2, double> getMouseCoords();

private:
    bool mIsAppRunning;
    Window mWindow;
    VulkanRenderer mRenderer {mWindow};
};

}