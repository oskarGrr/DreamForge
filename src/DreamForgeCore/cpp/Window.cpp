#include <iostream>
#include <format>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Window.hpp"
#include "Logging.hpp" 
#include "errorHandling.hpp"

namespace DF
{

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    bool* frameBuffResizedFlag = static_cast<bool*>(glfwGetWindowUserPointer(window));
    *frameBuffResizedFlag = true;
}

Window::Window(int width, int height) : mWidth{width}, mHeight{height}
{
    glfwInit();

    //pass in a callback function that will log glfw errors if they occur
    glfwSetErrorCallback(errorHandlerCallbackglfw);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    mWindow = glfwCreateWindow(mWidth, mHeight, mTitle, nullptr, nullptr);

    glfwSetWindowUserPointer(mWindow, &mFrameBuffResized);
    glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
}

Window::~Window()
{
    glfwDestroyWindow(this->mWindow);
    glfwTerminate();
}

VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const
{
    VkSurfaceKHR retval {VK_NULL_HANDLE};

    if(glfwCreateWindowSurface(instance, mWindow, nullptr, &retval) != VK_SUCCESS)
        throw SystemInitException("could not create vulkan surface with glfwCreateWindowSurface");

    return retval;
}

void Window::maximize()
{
    glfwMaximizeWindow(mWindow);
}

void Window::displayTitleFPS(double dt)
{
    static double avgDt {dt};
    float const alpha {0.0009f};
    avgDt = avgDt * (1-alpha) + dt * alpha;
    glfwSetWindowTitle(mWindow, std::format("FPS: {}", (U32)(1/avgDt)).c_str());
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(mWindow);
}

}