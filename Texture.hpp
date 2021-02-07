/*
** EPITECH PROJECT, 2020
**
** File description:
** Texture.hpp
*/

#ifndef TEXTURE_HPP_
#define TEXTURE_HPP_

#include <GL/glew.h>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>

#define TEX GL_TEXTURE_RECTANGLE

extern bool rescale;
extern int gwidth;
extern int gheight;

class GLFWwindow;

class Texture {
public:
    Texture(int width = 0, int height = 0, int depth = 1, int channels = 3);
    virtual ~Texture();
    Texture(const Texture &cpy) = delete;
    Texture &operator=(const Texture &src) = delete;

    // Start binding dependency
    void bind();
    // Stop binding dependency
    void unbind();
    // Update texture with datas
    void update(unsigned char *data);
    // Update texture with file content
    bool load(const std::string &filename);
    // Build mipmaps
    void buildMipmaps();
    // Get texture handle
    GLuint get() {return tex;}
    // Unoptimal rescaling method
    unsigned char *rescaling(unsigned char *);
    // Block texture bindings
    static void lock();
    // Unblock texture bindings
    static void unlock();
private:
    int width, height, depth, channels;
    GLuint tex = 0;
    unsigned char* image_data = nullptr;
    static GLuint bound;
    static std::recursive_mutex mtx;
};

#endif /* TEXTURE_HPP_ */
