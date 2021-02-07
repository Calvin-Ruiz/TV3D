/*
** EPITECH PROJECT, 2020
**
** File description:
** Texture.cpp
*/
#include "Texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Graphics.hpp"
#include <GLFW/glfw3.h>

GLuint Texture::bound = 0;
std::recursive_mutex Texture::mtx;

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

Texture::Texture(int width, int height, int depth, int channels) : width(width), height(height), depth(depth), channels(channels)
{
    lock();
    glGenTextures(1, &tex);
    if (tex == 0) {
        std::cerr << "Failed to create texture\n";
        catchError();
    }
    glBindTexture(TEX, tex);
    glTexParameteri(TEX, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(TEX, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mtx.unlock();
}

Texture::~Texture()
{
    glDeleteTextures(1, &tex);
}

void Texture::bind()
{
    lock();
    if (bound != tex) {
        glBindTexture(TEX, tex);
        bound = tex;
    }
}

void Texture::unbind()
{
    mtx.unlock();
}

void Texture::update(unsigned char *data)
{
    bind();
    if (rescale) {
        data = rescaling(data);
        glTexImage2D(TEX, 0, GL_RGB, gwidth, gheight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        delete data;
    } else
        glTexImage2D(TEX, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    unbind();
}

bool Texture::load(const std::string &filename)
{
    unsigned char *image_data = stbi_load(filename.c_str(), &width, &height, &depth, channels);

    if (image_data) {
        update(image_data);
        stbi_image_free(image_data);
        return true;
    } else {
        std::cerr << "Texture: could not load image '"  << filename << "'\n";
        return false;
    }
}

void Texture::buildMipmaps()
{
    bind();
    glGenerateMipmap(TEX);
    unbind();
}

void Texture::lock()
{
    mtx.lock();
    Graphics::instance->acquireOwnership();
}

void Texture::unlock()
{
    bound = 0;
    mtx.unlock();
}

unsigned char *Texture::rescaling(unsigned char *data)
{
    unsigned char *datas = new unsigned char[gwidth * gheight * 3];
    for (long j = 0; j < gheight; ++j) {
        for (long i = 0; i < gwidth; ++i) {
            datas[(i + j * gwidth) * 3] = data[(i * width / gwidth + j * gwidth * height / gheight) * 3];
            datas[(i + j * gwidth) * 3 + 1] = data[(i * width / gwidth + j * gwidth * height / gheight) * 3 + 1];
            datas[(i + j * gwidth) * 3 + 2] = data[(i * width / gwidth + j * gwidth * height / gheight) * 3 + 2];
        }
    }
    return datas;
}
