#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <format>

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

    //ImGuiIO& io = ImGui::GetIO();
    //if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    ImGui::UpdatePlatformWindows();
    //    ImGui::RenderPlatformWindowsDefault();
    //}

    //ImGui::EndFrame();

    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::chrono::seconds::period>(end - frameStartTime).count();
}

DreamForgeApp::DreamForgeApp()
    try : mWindow{}, mIsAppRunning{true}
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    mRenderer.initImguiRenderInfo();

    ScriptingEngine scriptingEngine;
    ScriptingEngine::printCILTypes(scriptingEngine.LoadCILAssembly(
        "resources/testScripts/testDLL.dll"));

    Logger::get().stdoutInfo("Dream forge engine initialized");
}
catch(SystemInitException const& e)
{
    Logger::get().stdoutError(e.what());
    Logger::get().stdoutError("error encountered while initializing the engine. shutting down");
    std::exit(-1);
};

void DreamForgeApp::run()
{
    double dt{};
    double runningAvg{};
    while( ! mWindow.shouldClose() )
    {
        auto startTime {startOfLoop(dt)};

        runningAvg = runningAvg * .99 + dt * 0.01;
        glfwSetWindowTitle(mWindow.getRawWindow(),
            std::format("FPS: {}", static_cast<unsigned>(1/runningAvg)).c_str());

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


DreamForgeApp::guiContext::guiContext(NonOwningPtr<GLFWwindow> wnd)
{
    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(wnd, true);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
}

DreamForgeApp::guiContext::~guiContext()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

} //end namespace DF