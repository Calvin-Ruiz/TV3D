/*
** EPITECH PROJECT, 2020
**
** File description:
** Graphics.cpp
*/
#include "Graphics.hpp"
#include "Core.hpp"
//#include "Buffer.hpp"
#include "VertexArray.hpp"
#include "TextureLoader.hpp"
#include "Texture.hpp"
#include "shader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "vecmath.hpp"

Graphics *Graphics::instance;

Graphics::Graphics(GLFWwindow *window, std::array<TextureLoader *, 8> &texLoader) : window(window), texLoader(texLoader)
{
    instance = this;
}

Graphics::~Graphics() {}

static void catchError()
{
    GLenum error = GL_NO_ERROR;
    do {
        GLenum error = glGetError();
        switch (error) {
            case GL_NO_ERROR:
                break;
            case GL_INVALID_ENUM:
                std::cout << "Invalid enum\n";
                break;
            case GL_INVALID_VALUE:
                std::cout << "Invalid value\n";
                break;
            case GL_INVALID_OPERATION:
                std::cout << "Invalid operation\n";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cout << "Invalid framebuffer operation\n";
                break;
            default:
                std::cout << "Unhandled error\n";
                break;
        }
    } while (error);
}

void Graphics::initialize()
{
    height = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;
    Texture::lock();
    glewInit();
    shader = std::make_unique<shaderProgram>();
    shader->init("shader.vert", "shader.frag");
    shader->setUniformLocation({"position", "displayHeight", "hasAlpha", "brightPos"});
    shader->use();
    shader->setUniform("position", Vec2f(0, 0));
    shader->setUniform("displayHeight", height);
    shader->setUniform("hasAlpha", GL_FALSE);
    shader->setUniform("brightPos", 0);

    vertex = std::make_unique<VertexArray>(GL_STATIC_DRAW);
    vertex->addComponent(2); // pos
    vertex->addComponent(2); // tex
    vertex->resize(4);
    float datas[] = {
        -1, 1, 0, 0,
        1, 1, 1, 0,
        -1, -1, 0, 1,
        1, -1, 1, 1
    };
    vertex->update(datas);
    vertex->bindLocation();
    vertex->bind();
    Texture::unlock();
    ready = true;
}

void Graphics::refresh()
{
    for (auto &tex : texLoader) {
        if (!tex->hasFrame())
            return;
    }
    //glClear(GL_COLOR_BUFFER_BIT); // not needed while draw cover the whole surface
    Texture::lock();
    for (auto &tex : texLoader) {
        tex->bindFrame();
    }
    if (effect)
        shader->setUniform("brightPos", (float) (height - frame++));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    Texture::unlock();
    if (++frame >= height + 5)
        frame = -5;
    glfwSwapBuffers(window);
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

void Graphics::acquireOwnership()
{
    static std::thread::id owner;

    if (owner != std::this_thread::get_id()) {
        glfwMakeContextCurrent(window);
        owner = std::this_thread::get_id();
    }
}
