#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_impl_sdl2.h"
#include "Window.hpp"
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "themeTests.hpp"

Window::Window(uint32_t sdlFlags, uint32_t windowFlags)
{
    SDL_Init(sdlFlags);

    m_window = SDL_CreateWindow
    (
        m_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_width,
        m_height,
        windowFlags
    );

    if(!m_window)
    {
        std::cerr << SDL_GetError() << '\n';
        //TODO make error logger
        return;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    TTF_Init();
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
    Mix_Init
    (
        MIX_INIT_MP3 | MIX_INIT_FLAC |
        MIX_INIT_OGG | MIX_INIT_MID
    );

    setImGuiSettings();
}

Window::~Window()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_DestroyWindow(m_window);
    SDL_DestroyRenderer(m_renderer);
    SDL_Quit();
}

void Window::maximize()
{
    SDL_MaximizeWindow(m_window);
}

void Window::setImGuiSettings()
{
    //TODO: modifiable themes in a settings menu.

    //themeGreenDark();
    //themeLight();
    //themeSuperDark();
    themeDarkRedOrange();

    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/fonts/ebrima.ttf", 16.0);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}