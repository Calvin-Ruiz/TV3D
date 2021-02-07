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
class GLFWwindow;
class Vulkan;

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
    static std::unique_ptr<Vulkan> vulkan;
    // Used by class Vulkan
    std::array<TextureLoader *, 8> &getLoaders() {return texLoader;}
private:
    GLFWwindow *window;
    std::array<TextureLoader *, 8> &texLoader;
    int height;
    int frame = -5;
    float *brightPos;
};

#endif /* GRAPHICS_HPP_ */
