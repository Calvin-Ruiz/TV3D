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
#include <vulkan/vulkan.h>

class Texture;
class Vulkan;

class TextureLoader : public ThreadedModule {
public:
    TextureLoader(Vulkan *master, int id, const std::string &path, const std::string &path2);
    virtual ~TextureLoader();
    TextureLoader(const TextureLoader &cpy) = delete;
    TextureLoader &operator=(const TextureLoader &src) = delete;

    virtual void initialize() override;
    virtual void update() override;
    bool hasFrame();
    Texture *getFrame();
    Texture *getFrame(int id);
    void releaseFrame();
private:
    Vulkan *master;
    int id;
    std::string path;
    std::string path2;
    std::vector<std::unique_ptr<Texture>> textures;
    int frameID = 0;
    unsigned char readIndex = 0;
    unsigned char writeIndex = 0;
    std::atomic<unsigned char> count;
    VkQueue queue;
    VkCommandPool pool;
    VkCommandBuffer cmds[4];
    bool isCompiled;
};

#endif /* TEXTURELOADER_HPP_ */
