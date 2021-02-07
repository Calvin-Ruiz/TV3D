/*
** EPITECH PROJECT, 2020
**
** File description:
** Texture.hpp
*/

#ifndef TEXTURE_HPP_
#define TEXTURE_HPP_

#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>

extern bool rescale;
extern int gwidth;
extern int gheight;
extern bool useDump;

class GLFWwindow;
class Vulkan;

class Texture {
public:
    Texture(Vulkan *master, VkQueue queue, VkCommandBuffer cmd, int width, int height,
        VkDeviceMemory &memory, void *data, VkDeviceSize offset, int &size,
        VkBuffer buffer, void *bufferPtr, VkDeviceSize bufferOffset);
    virtual ~Texture();
    Texture(const Texture &cpy) = delete;
    Texture &operator=(const Texture &src) = delete;

    // Initialize update method
    void initUpdate(VkDeviceSize bufferOffset);
    // Update texture
    void update();
    // Load input file content in texture, if useDump is true, save texture memory to output file
    bool compile(const std::string &input, const std::string &output);
    // Write file content to texture memory
    bool load(const std::string &filename);
    // Get texture handle
    VkImage get() {return image;}
    // Get texture view
    VkImageView getView() {return imageView;}
    // Get fence
    VkFence getFence() {return fence;}
    // Block texture bindings
    static void lock();
    // Unblock texture bindings
    static void unlock();
private:
    Vulkan *master;
    int width, height, depth = 1, channels = 3;
    int size;
    void *ptr;
    VkBuffer buffer;
    void *buffPtr;
    VkImage image;
    VkImageView imageView;
    VkQueue queue;
    VkCommandBuffer cmd;
    VkSubmitInfo submit{};
    VkFence fence;
    static std::recursive_mutex mtx;
};

#endif /* TEXTURE_HPP_ */
