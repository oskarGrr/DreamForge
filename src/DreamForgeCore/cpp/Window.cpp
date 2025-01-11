#include <iostream>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "Window.hpp"
#include "themeTests.hpp"
#include "Logging.hpp" 
#include "errorHandling.hpp"

Window::Window(int width, int height) : m_width{width}, m_height{height}
{
    glfwInit();

    //pass in a callback function that will log glfw errors if they occur
    glfwSetErrorCallback(errorHandlerCallbackglfw);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);

    //setImGuiSettings();
}

Window::~Window()
{
    glfwDestroyWindow(this->m_window);
    glfwTerminate();
}

void Window::maximize()
{
    glfwMaximizeWindow(m_window);
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void Window::setImGuiSettings()
{
    //TODO: modifiable themes in a settings menu.

    //themeGreenDark();
    //themeLight();
    //themeSuperDark();
    //themeDarkRedOrange();

   /* auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/fonts/ebrima.ttf", 16.0);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;*/
}