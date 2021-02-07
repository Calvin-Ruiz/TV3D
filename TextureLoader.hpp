/*
** EPITECH PROJECT, 2020
**
** File description:
** TextureLoader.hpp
*/

#ifndef TEXTURELOADER_HPP_
#define TEXTURELOADER_HPP_

#include <iostream>
#include <string>
#include <memory>
#include "ThreadedModule.hpp"
#include <atomic>
#include <vector>

class Texture;

class TextureLoader : public ThreadedModule {
public:
    TextureLoader(int id, const std::string &path);
    virtual ~TextureLoader();
    TextureLoader(const TextureLoader &cpy) = delete;
    TextureLoader &operator=(const TextureLoader &src) = delete;

    virtual void initialize() override;
    virtual void update() override;
    bool hasFrame();
    void bindFrame();
    void releaseFrame();
private:
    int id;
    std::string path;
    std::vector<std::unique_ptr<Texture>> textures;
    int frameID = 0;
    unsigned char readIndex = 0;
    unsigned char writeIndex = 0;
    std::atomic<unsigned char> count;
};

#endif /* TEXTURELOADER_HPP_ */
