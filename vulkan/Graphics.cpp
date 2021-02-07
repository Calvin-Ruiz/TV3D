/*
** EPITECH PROJECT, 2020
**
** File description:
** Graphics.cpp
*/
#include "Graphics.hpp"
#include "Core.hpp"
#include "TextureLoader.hpp"
#include "Texture.hpp"
#include "Vulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <thread>
#include "vecmath.hpp"
#include <string.h>

Graphics *Graphics::instance = nullptr;
std::unique_ptr<Vulkan> Graphics::vulkan;

Graphics::Graphics(GLFWwindow *window, std::array<TextureLoader *, 8> &texLoader) : window(window), texLoader(texLoader)
{
    instance = this;
    vulkan = std::make_unique<Vulkan>(glfwGetVideoMode(glfwGetPrimaryMonitor())->width, glfwGetVideoMode(glfwGetPrimaryMonitor())->height, window);
}

Graphics::~Graphics() {}

void Graphics::initialize()
{
    auto uniform = vulkan->getUniform();
    height = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;

    uniform->position = Vec2f(0, 0);
    uniform->displayHeight = height;
    uniform->hasAlpha = VK_FALSE;
    uniform->brightPos = 0;
    brightPos = &uniform->brightPos;

    float datas[] = {
        -1, 1, 0, 0,
        1, 1, 1, 0,
        -1, -1, 0, 1,
        1, -1, 1, 1
    };
    char *data = reinterpret_cast<char *>(uniform) + sizeof(*uniform);
    memcpy(data, datas, 16 * sizeof(float));
    ready = true;
}

void Graphics::refresh()
{
    for (auto &tex : texLoader) {
        if (!tex->hasFrame()) {
            while (vulkan->pushQueue());
            return;
        }
    }
    while (vulkan->pushQueue());
    vulkan->waitFrame();
    if (effect)
        *brightPos = (height - frame++);
    vulkan->drawFrame();
    if (++frame >= height + 5)
        frame = -5;
    for (auto &tex : texLoader) {
        tex->releaseFrame();
    }
}

void Graphics::update()
{
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        Core::core->setPause(true);
    if (glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        ready = false;
}

void Graphics::onPause()
{
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        Core::core->setPause(false);
    if (glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        ready = false;
}
