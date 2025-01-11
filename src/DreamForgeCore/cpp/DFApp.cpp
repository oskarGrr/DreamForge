#include <chrono>
#include <iostream>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Window.hpp"
#include "Scripting.hpp"
#include "DFApp.hpp"
#include "Logging.hpp"
#include "Components.hpp"
#include "HelpfulTypeAliases.hpp"
#include "ECS.hpp"

template <typename VecType, U32 R>
auto& operator<<(std::ostream& os, glm::vec<R, VecType> const& vec)
{
    for(U32 i = 0; i < R; ++i) { std::cout << vec[i] << ", "; }
    return os;
}

//SDL_Init called in Window ctor
DreamForgeApp::DreamForgeApp()
    : m_window{},
      m_isAppRunning{true}
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    DFLog::get().stdoutInfo("Dream forge engine initialized");
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

void DreamForgeApp::runApp()
{
    U32 extensionCount {};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    DFLog::get().fmtStdoutInfo("{} extensions supported", extensionCount);

    ScriptingEngine scriptingEngine;   
    ScriptingEngine::printCILTypes(scriptingEngine.LoadCILAssembly(
        "resources/testScripts/testDLL.dll"));

    F64 dt{0.0};
    while( ! m_window.shouldClose() )
    {
        auto startTime { renderBeginFrame() };

        processWindowEvents();

        dt = renderEndFrame(startTime);
    }
}

void DreamForgeApp::processWindowEvents()
{
    glfwPollEvents();
}