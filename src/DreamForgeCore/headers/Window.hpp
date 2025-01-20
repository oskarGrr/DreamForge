#pragma once
#include <cstdint>
#include <GLFW/glfw3.h>
#include "HelpfulTypeAliases.hpp"

namespace DF
{

//wrapper for SDL, SDL satellites, and imgui stuff.
//Not a singleton to avoid problems in the future like SIOF,
//but should only be instantiated once like a singleton.
class Window
{
public:

    constexpr static int INITIAL_WINDOW_HEIGHT {990};
    constexpr static int INITIAL_WINDOW_WIDTH  {1760};

    Window(int width = INITIAL_WINDOW_WIDTH, int height = INITIAL_WINDOW_HEIGHT);
    ~Window();

    Window(Window const&)=delete;
    Window(Window&&)=delete;
    Window& operator=(Window const&)=delete;
    Window& operator=(Window&&)=delete;

    void maximize();

    auto getWidth()  const {return m_width;}
    auto getHeight() const {return m_height;}

    bool shouldClose() const;

private:
    GLFWwindow* m_window {nullptr};
    const char* const m_title {"Dream Forge"};
    int m_width, m_height;

    void setImGuiSettings();
};

}