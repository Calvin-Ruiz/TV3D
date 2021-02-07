/*
** EPITECH PROJECT, 2020
**
** File description:
** TextureLoader.cpp
*/
#include "TextureLoader.hpp"
#include "Texture.hpp"
#include "Vulkan.hpp"

TextureLoader::TextureLoader(Vulkan *master, int id, const std::string &path, const std::string &path2) :
    master(master), id(id), path(path), path2(path2), count(0)
{
    queue = master->getQueue(id);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = master->getQueueIndex();
    if (vkCreateCommandPool(master->refDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création d'une command pool !");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 4;
    allocInfo.commandPool = pool;
    if (vkAllocateCommandBuffers(master->refDevice, &allocInfo, cmds) != VK_SUCCESS) {
        throw std::runtime_error("échec de l'allocation de command buffers!");
    }

    isCompiled = useDump;
}

TextureLoader::~TextureLoader()
{
    textures.clear();
    vkDestroyCommandPool(master->refDevice, pool, nullptr);
}

void TextureLoader::initialize()
{
    TexData data = master->getDataBlock(id);
    VkDeviceSize offset = 0;
    VkDeviceSize buffOffset = 0;
    int size = data.size;
    Texture::lock();
    for (int i = 0; i < 4; ++i) {
        textures.emplace_back(new Texture(master, queue, cmds[i], gwidth, gheight,
            data.memory, data.ptr, offset, size,
            data.buffer, data.buffPtr, buffOffset));
            offset += size;
            data.ptr += size;
            buffOffset += gwidth * gheight * 3;
            data.buffPtr += gwidth * gheight * 3;
    }
    master->setDataSize(id, size);
    Texture::unlock();
    ready = true;
}

void TextureLoader::update()
{
    if (count == 3) // Don't overwrite acquired texture
        return;
    std::string num = std::to_string(frameID++);
    std::string filler;
    filler.resize(4 - num.size(), '0');
    std::string filename = path + filler + num + ".png";
    std::string output = path2 + filler + num + ".dat";
    bool ret;
    if (isCompiled) {
        ret = textures[writeIndex]->load(output);
    } else {
        ret = textures[writeIndex]->compile(filename, output);
    }
    if (ret) {
        ++count;
        ++writeIndex;
        writeIndex &= 3;
    } else {
        std::cout << "No frame '" + (isCompiled ? output : filename) + "' found\n";
        if (isCompiled && frameID == 1)
            isCompiled = false; // We expect compiled form, but it is not compiled yet
        frameID = 0;
    }
}

bool TextureLoader::hasFrame()
{
    return (count > 0);
}

Texture *TextureLoader::getFrame()
{
    return textures[readIndex].get();
}

Texture *TextureLoader::getFrame(int id)
{
    return textures[id].get();
}

void TextureLoader::releaseFrame()
{
    --count;
    ++readIndex;
    readIndex &= 3;
}
