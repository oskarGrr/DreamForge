#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "DFApp.hpp"
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

namespace DF
{

[[nodiscard]] std::chrono::steady_clock::time_point 
DreamForgeApp::startOfLoop(double dt)
{
    const auto start = std::chrono::steady_clock::now();

    //ImGui_ImplVulkan_NewFrame();
    //ImGui_ImplGlfw_NewFrame();
    //ImGui::NewFrame();

    mRenderer.update(dt, getMousePos());

    processWindowEvents();

    return start;
}

double DreamForgeApp::endOfLoop(std::chrono::steady_clock::time_point const frameStartTime)
{
    //ImGui::Render();
    //ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    //SDL_RenderPresent(renderer);
    //ImGui::EndFrame();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - frameStartTime).count() / 1.0E9;
}



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
catch(std::exception const& e)
{
    Logger::get().stdoutError(e.what());
}

void DreamForgeApp::run()
{
    double dt{0.0};
    while( ! mWindow.shouldClose() )
    {
        auto startTime {startOfLoop(dt)};

        dt = endOfLoop(startTime);
    }
}

void DreamForgeApp::processWindowEvents()
{
    glfwPollEvents();
}

glm::vec<2, double> DreamForgeApp::getMousePos()
{
    double x, y;
    glfwGetCursorPos(mWindow.getRawWindow(), &x, &y);
    return {x, y};
}

} //end namespace DF