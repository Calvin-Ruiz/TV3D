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

Graphics::Graphics(GLFWwindow *window, std::array<TextureLoader *, 8> &texLoader, bool debug) : window(window), texLoader(texLoader)
{
    instance = this;
    int width = glfwGetVideoMode(glfwGetPrimaryMonitor())->width;
    int height = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;
    std::cout << "Screen of " << width << "x" << height << " detected\n";
    vulkan = std::make_unique<Vulkan>(width, height, window, debug);
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
        -1, -1, 0, 0,
        1, -1, 1, 0,
        -1, 1, 0, 1,
        1, 1, 1, 1
    };
    void *data = reinterpret_cast<void *>(uniform) + sizeof(UniformBlock);
    memcpy(data, datas, 16 * sizeof(float));
    ready = true;
}

void Graphics::refresh()
{
    std::cout << "Refresh sequence\n";
    for (auto &tex : texLoader) {
        if (!tex->hasFrame()) {
            while (vulkan->pushQueue());
            return;
        }
    }
    bool hasSubmitted = vulkan->pushQueue(); // assume there is at least one task to submit if submitting task
    while (vulkan->pushQueue());
    if (hasSubmitted)
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
