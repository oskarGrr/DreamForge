#pragma once
#include <string>
#include <cstdint>
#include "HelpfulTypeAliases.hpp"

struct SDL_Window;
struct SDL_Renderer;

#define INITIAL_WINDOW_HEIGHT 990
#define INITIAL_WINDOW_WIDTH  1760

//wrapper for SDL, SDL satellites, and imgui stuff.
//Not a singleton to avoid problems in the future like SIOF,
//but should only be instantiated once like a singleton.
class Window
{
public:
    Window()=delete;
    Window(uint32_t sdlFlags, uint32_t windowFlags);
    ~Window();

    Window(Window const&)=delete;
    Window(Window&&)=delete;
    Window& operator=(Window const&)=delete;
    Window& operator=(Window&&)=delete;

    //TODO make renderer class and use sdl with vulkan/opengl
    SDL_Renderer* getCurrentRenderer() {return m_renderer;}

    void maximize();

    auto getWidth()  const {return m_width;}
    auto getHeight() const {return m_height;}

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};//TODO make renderer class and use sdl with vulkan/opengl
    const std::string m_title{"j^2 engine"};
    U32 m_width{INITIAL_WINDOW_WIDTH}, m_height{INITIAL_WINDOW_HEIGHT};

    void setImGuiSettings();
};