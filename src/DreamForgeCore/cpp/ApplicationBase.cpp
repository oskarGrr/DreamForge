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

#include "ApplicationBase.hpp"
#include "Window.hpp"
#include "Scripting.hpp"
#include "ApplicationBase.hpp"
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
ApplicationBase::startOfLoop()
{
    const auto start = std::chrono::steady_clock::now();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    processWindowEvents();
    return start;
}

double ApplicationBase::endOfLoop(std::chrono::steady_clock::time_point const frameStartTime)
{
    ImGui::Render();

    mRenderer.update(mFrameTime, getMousePos());

    //ImGuiIO& io = ImGui::GetIO();
    //if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    ImGui::UpdatePlatformWindows();
    //    ImGui::RenderPlatformWindowsDefault();
    //}

    ImGui::EndFrame();

    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::chrono::seconds::period>(end - frameStartTime).count();
}

ApplicationBase::ApplicationBase()
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
    std::exit(EXIT_FAILURE);
}
catch(std::exception const& e)
{
    Logger::get().stdoutError(e.what());
    std::exit(EXIT_FAILURE);
}

void ApplicationBase::run()
{
    while( ! mWindow.shouldClose() )
    {
        auto startTime {startOfLoop()};

        mWindow.displayTitleFPS(mFrameTime);
        imguiDraw();//call the app defined override for imguiDraw

        mFrameTime = endOfLoop(startTime);
    }
}

void ApplicationBase::processWindowEvents()
{
    glfwPollEvents();
}

glm::vec<2, double> ApplicationBase::getMousePos()
{
    double x, y;
    glfwGetCursorPos(mWindow.getRawWindow(), &x, &y);
    return {x, y};
}


ApplicationBase::guiContext::guiContext(NonOwningPtr<GLFWwindow> wnd)
{
    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(wnd, true);
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    /*io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;*/
}

ApplicationBase::guiContext::~guiContext()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

} //end namespace DF