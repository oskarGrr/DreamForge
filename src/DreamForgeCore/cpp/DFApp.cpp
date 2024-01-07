#include <chrono>
#include <iostream>
#include "Window.hpp"
#include "Scripting.hpp"
#include "DFApp.hpp"
#include "SDL.h"
#include "SDL_image.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_impl_sdl2.h"
#include "glm/glm.hpp"
#include "Texture.hpp"
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
JsquaredApp::JsquaredApp()
    : m_window{SDL_INIT_EVERYTHING, SDL_WINDOW_RESIZABLE},
      m_isAppRunning{true}
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    DFLog::get().stdoutInfo("J^2 engine initialized");
}

static void renderBeginFrame(SDL_Renderer* renderer)
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

static void renderEndFrame(SDL_Renderer* renderer)
{
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
    ImGui::EndFrame();
}

void JsquaredApp::runApp()
{
    ScriptingEngine scriptingEngine;   

    //testing mono stuff
    ScriptingEngine::printCILTypes(scriptingEngine.LoadCILAssembly(
        "resources/testScripts/testDLL.dll"));

    auto renderer = m_window.getCurrentRenderer();

    F64 dt{0.0};
    while(m_isAppRunning)
    {
        const auto start = std::chrono::steady_clock::now();
        renderBeginFrame(renderer);
        processSDLEvents();

        ImGui::ShowDemoWindow();

        renderEndFrame(renderer);
        const auto end = std::chrono::steady_clock::now();
        dt = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1.0E9;
    }
}

void JsquaredApp::processSDLEvents()
{
    SDL_Event evnt;
    while(SDL_PollEvent(&evnt))
    {
        ImGui_ImplSDL2_ProcessEvent(&evnt);
        switch(evnt.type)
        {
        case SDL_QUIT:
        {
            m_isAppRunning = false;
            break;
        }
        case SDL_KEYUP:
        {
            if(evnt.key.keysym.sym == SDLK_F10)
                m_window.maximize();
        }
        }
    }
}