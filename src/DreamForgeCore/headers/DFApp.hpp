#pragma once

#include "Window.hpp"
#include "df_export.hpp"

class DF_DLL_API DreamForgeApp
{
public:
    DreamForgeApp();
    virtual ~DreamForgeApp()=default;
    
    DreamForgeApp(DreamForgeApp const&)=delete;
    DreamForgeApp(DreamForgeApp&&)=delete;
    DreamForgeApp& operator=(DreamForgeApp const&)=delete;
    DreamForgeApp& operator=(DreamForgeApp&&)=delete;
    
    void runApp();

    void processWindowEvents();

private:
    Window m_window;
    bool m_isAppRunning;
};