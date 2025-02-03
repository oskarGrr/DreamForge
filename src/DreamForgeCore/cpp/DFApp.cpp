#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>

#include "Window.hpp"
#include "Scripting.hpp"
#include "DFApp.hpp"
#include "Logging.hpp"
#include "Components.hpp"
#include "HelpfulTypeAliases.hpp"
#include "ECS.hpp"
#include "errorHandling.hpp"

template <typename VecType, U32 R>
auto& operator<<(std::ostream& os, glm::vec<R, VecType> const& vec)
{
    for(U32 i = 0; i < R; ++i) { std::cout << vec[i] << ", "; }
    return os;
}

static std::chrono::steady_clock::time_point renderBeginFrame()
{
    const auto start = std::chrono::steady_clock::now();
    //ImGui_ImplSDLRenderer2_NewFrame();
    //ImGui_ImplSDL2_NewFrame();
    //ImGui::NewFrame();
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);

    


    return start;
}

static F64 renderEndFrame(std::chrono::steady_clock::time_point const frameStartTime)
{
    //ImGui::Render();
    //ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    //SDL_RenderPresent(renderer);
    //ImGui::EndFrame();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - frameStartTime).count() / 1.0E9;
}

namespace DF
{

DreamForgeApp::DreamForgeApp()
    try
    : mWindow{},
      mIsAppRunning{true}
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    ScriptingEngine scriptingEngine;
    ScriptingEngine::printCILTypes(scriptingEngine.LoadCILAssembly(
        "resources/testScripts/testDLL.dll"));

    Logger::get().stdoutInfo("Dream forge engine initialized");
}
catch(SystemInitException const& e)
{
    Logger::get().stdoutError(e.what());
}

void DreamForgeApp::run()
{
    F64 dt{0.0};
    while( ! mWindow.shouldClose() )
    {
        auto startTime { renderBeginFrame() };

        processWindowEvents();
        mRenderer.update();

        dt = renderEndFrame(startTime);
    }
}

void DreamForgeApp::processWindowEvents()
{
    glfwPollEvents();
}

} //end namespace DF