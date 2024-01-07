#pragma once

#include "Window.hpp"
#include "df_export.hpp"

class DF_DLL_API JsquaredApp
{
public:
    JsquaredApp();
    virtual ~JsquaredApp()=default;
    
    JsquaredApp(JsquaredApp const&)=delete;
    JsquaredApp(JsquaredApp&&)=delete;
    JsquaredApp& operator=(JsquaredApp const&)=delete;
    JsquaredApp& operator=(JsquaredApp&&)=delete;
    
    void runApp();

    void processSDLEvents();

private:
    Window m_window;
    bool m_isAppRunning;
};