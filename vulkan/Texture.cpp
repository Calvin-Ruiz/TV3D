/*
** EPITECH PROJECT, 2020
**
** File description:
** Texture.cpp
*/
#include "Texture.hpp"
#include "Vulkan.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Graphics.hpp"
#include <GLFW/glfw3.h>
#include <fstream>
#include <string.h>

std::recursive_mutex Texture::mtx;

Texture::Texture(Vulkan *master, VkQueue queue, VkCommandBuffer cmd, int width, int height,
    VkDeviceMemory &memory, void *data, VkDeviceSize offset, int &size,
    VkBuffer buffer, void *bufferPtr, VkDeviceSize bufferOffset) :
    master(master), width(width), height(height), ptr(data), buffer(buffer), buffPtr(bufferPtr), queue(queue), cmd(cmd)
{
    // Create texture
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(master->refDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::cerr << "Faild to create VkImage\n";
        return;
    }
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(master->refDevice, image, &memRequirements);
    size = memRequirements.size + ((memRequirements.size % memRequirements.alignment) == 0 ? 0 : memRequirements.alignment - memRequirements.size % memRequirements.alignment);
    this->size = size;

    //std::cout << "Bind image of size " << size << " at offset " << offset << " aligned at " << memRequirements.alignment << std::endl;
    if (vkBindImageMemory(master->refDevice, image, memory, offset) != VK_SUCCESS) {
        std::cerr << "Faild to bind memory to VkImage\n";
        vkDestroyImage(master->refDevice, image, nullptr);
        return;
    }
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(master->refDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        std::cerr << "Faild to create VkImageView\n";
        vkDestroyImage(master->refDevice, image, nullptr);
        return;
    }
    master->setLayout(image, queue); // set layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    initUpdate(bufferOffset);
}

Texture::~Texture()
{
    // Resources are not destroyed yet, no time for this
}

void Texture::initUpdate(VkDeviceSize bufferOffset)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("erreur au début de l'enregistrement d'un command buffer!");
    }
    VkImageMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

    VkImageSubresourceLayers subresource {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    VkBufferImageCopy region {(VkDeviceSize) bufferOffset, 0, 0, subresource, {0, 0, 0}, {(uint32_t) width, (uint32_t) height, 1}};
    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = barrier.newLayout;
    barrier.srcAccessMask = barrier.dstAccessMask;
    barrier.dstAccessMask = 0;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(master->refDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence.");
    }
}

void Texture::update()
{
    if (queue == VK_NULL_HANDLE)
        master->addSubmit(&submit, fence);
    else
        vkQueueSubmit(queue, 1, &submit, fence);
}

bool Texture::compile(const std::string &input, const std::string &output)
{
    unsigned char *image_data = stbi_load(input.c_str(), &width, &height, &depth, channels);

    if (image_data) {
        memcpy(buffPtr, image_data, width * height * channels);
        update();
        if (useDump) {
            vkQueueWaitIdle(queue);
            std::ofstream file(output, std::ios::binary);
            if (file.is_open()) {
                file.write((char *) ptr, size);
            } else {
                std::cerr << "Texture: could not save image '"  << output << "'\n";
                return false;
            }
        }
        return true;
    } else {
        std::cerr << "Texture: could not load image '"  << input << "'\n";
        return false;
    }
}

bool Texture::load(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);

    if (file.is_open()) {
        file.read((char *) ptr, size); // Directly write to texture
        return true;
    } else {
        std::cerr << "Texture: could not load image '"  << filename << "'\n";
        return false;
    }
}

void Texture::lock()
{
    mtx.lock();
}

void Texture::unlock()
{
    mtx.unlock();
}
