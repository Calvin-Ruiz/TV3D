/*
** EPITECH PROJECT, 2020
**
** File description:
** Graphics.hpp
*/

#ifndef GRAPHICS_HPP_
#define GRAPHICS_HPP_

#include "ThreadedModule.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <array>
extern bool effect;

class TextureLoader;
class shaderProgram;
class VertexArray;
class GLFWwindow;

class Graphics : public ThreadedModule {
public:
    Graphics(GLFWwindow *window, std::array<TextureLoader *, 8> &texLoader);
    virtual ~Graphics();
    Graphics(const Graphics &cpy) = delete;
    Graphics &operator=(const Graphics &src) = delete;

    virtual void initialize() override;
    virtual void refresh() override;
    virtual void update() override;
    virtual void onPause() override;
    static Graphics *instance;
    void acquireOwnership();
private:
    GLFWwindow *window;
    std::array<TextureLoader *, 8> &texLoader;
    std::unique_ptr<shaderProgram> shader;
    std::unique_ptr<VertexArray> vertex;
    int height;
    int frame = -5;
};

#endif /* GRAPHICS_HPP_ */
