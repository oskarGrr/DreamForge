#pragma once
#include <cstdint>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
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

    VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

    void maximize();

    auto getWidth()  const {return mWidth;}
    auto getHeight() const {return mHeight;}

    GLFWwindow* getRawWindow() const {return mWindow;}

    bool shouldClose() const;

private:
    GLFWwindow* mWindow {nullptr};
    const char* const mTitle {"Dream Forge"};
    int mWidth, mHeight;
};

}