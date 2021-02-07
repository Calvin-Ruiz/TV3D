/*
** EPITECH PROJECT, 2020
** TV3D
** File description:
** Vulkan.cpp
*/
#include "Vulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>
#include "Graphics.hpp"
#include "TextureLoader.hpp"
#include "Texture.hpp"

struct Vertex {
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(float) * 4;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(float) * 4;
        return attributeDescriptions;
    }
};

Vulkan::Vulkan(int width, int height, GLFWwindow *window) : refDevice(device), refRenderPass(renderPass), refSwapChainFramebuffers(swapChainFramebuffers), writePos(0), queueSize(0), prioritizedQueueSize(0)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    instanceExtension.insert(instanceExtension.begin(), glfwExtensions, glfwExtensions + glfwExtensionCount);

    initDevice("TV3D", "No Engine", window, true);
    initQueues();
    initSwapchain(width, height);
    createImageViews();
    createRenderPass();
    createFramebuffer();
    createTextureSampler();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDescriptorPool();
    initBuffers();
    if (!async) {
        syncSubmitQueue.resize(255);
        syncFenceQueue.resize(255);
        prioritizedSubmitQueue.resize(255);
    }
}

Vulkan::~Vulkan()
{
    vkDeviceWaitIdle(device);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyBuffer(device, buffers, nullptr);
    vkFreeMemory(device, bufferMemory, nullptr);
    vkDestroyCommandPool(device, singlePool, nullptr);
    vkDestroyCommandPool(device, pool, nullptr);
    for (auto &tex : texData) {
        vkDestroyBuffer(device, tex.buffer, nullptr);
        vkFreeMemory(device, tex.memory, nullptr);
    }
    for (auto &sem : acquired)
        vkDestroySemaphore(device, sem, nullptr);
    for (auto &sem : drawn)
        vkDestroySemaphore(device, sem, nullptr);
    vkDestroyDevice(device, nullptr);
    destroyDebug();
}

void Vulkan::waitFrame()
{
    int tmp = (frameID + 1) % 4;
    if (vkWaitForFences(device, 8, frameFences[tmp].data(), VK_TRUE, 10000000000) == VK_SUCCESS) {
        vkResetFences(device, 8, frameFences[tmp].data());
    } else {
        std::cerr << "Fatal: Timeout (10s) on frame waiting\n";
    }
}

void Vulkan::drawFrame()
{
    int tmp = (frameID + 1) % 4;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, acquired[tmp], VK_NULL_HANDLE, &frameID);
    if (frameID != tmp) {
        std::cerr << "Fatal: Async between planned and optained frame\n";
    }

    if (vkQueueSubmit(queues[8], 1, &submit[frameID], VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("échec de l'envoi d'un command buffer!");
    }
    // if (async) {
    // } else {
    //     std::cout << "submissions";
    //     prioritizedSubmitQueue[writePos++] = &submit[frameID];
    //     ++prioritizedQueueSize;
    // }

    // Display result
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &drawn[frameID];

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &frameID;
    presentInfo.pResults = nullptr; // Optionnel
    // while (prioritizedQueueSize > 0)
    //     pushQueue();
    vkQueuePresentKHR(queues[8], &presentInfo);
    //vkQueueWaitIdle(queues[8]);
}

bool Vulkan::pushQueue()
{
    if (async) {
        return false;
    }
    if (prioritizedQueueSize > 0) {
        vkQueueSubmit(queues[8], 1, prioritizedSubmitQueue[prioritizedReadPos++], VK_NULL_HANDLE);
        --prioritizedQueueSize;
        return true;
    }
    if (queueSize > 0) {
        vkQueueSubmit(queues[8], 1, syncSubmitQueue[readPos], syncFenceQueue[readPos]);
        ++readPos;
        --queueSize;
        return true;
    }
    return false;
}

void Vulkan::initBuffers()
{
    // Global resources : singlePool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = queueIndex;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &singlePool) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création d'une command pool !");
    }
    // Global Buffers : bufferMemory, buffers, buffPtr
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(UniformBlock) + sizeof(float) * 16;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffers) != VK_SUCCESS)
        throw std::runtime_error("Faild to create global buffer");
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffers, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Faild to allocate memory");
    if (vkMapMemory(device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &buffPtr) != VK_SUCCESS)
        throw std::runtime_error("Faild to map memory");
    if (vkBindBufferMemory(device, buffers, bufferMemory, 0) != VK_SUCCESS)
        std::cerr << "Faild to bind buffer memory.\n";

    // Data complex : texData
    texData.resize(8);
    const VkDeviceSize allocSize = 120*1024*1024; // Intel GPU doesn't allow allocate more than 1Go
    VkBufferCreateInfo bInfo{};
    bInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bInfo.size = ((long) gwidth)*((long)gheight)*3*4;
    bInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocInfo.allocationSize = allocSize;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    for (auto &tex : texData) {
        if (vkCreateBuffer(device, &bInfo, nullptr, &tex.buffer) != VK_SUCCESS)
            throw std::runtime_error("Faild to create buffer");
        vkGetBufferMemoryRequirements(device, tex.buffer, &memRequirements);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &tex.memory) != VK_SUCCESS)
            throw std::runtime_error("Faild to allocate memory");
        if (vkMapMemory(device, tex.memory, 0, VK_WHOLE_SIZE, 0, &tex.ptr) != VK_SUCCESS)
            throw std::runtime_error("Faild to map memory");
        tex.bufferOffset = allocSize - memRequirements.size;
        tex.bufferOffset -= tex.bufferOffset % memRequirements.alignment;
        //std::cout << "buffer of size " << bInfo.size << " using " << memRequirements.size << " at offset " << tex.bufferOffset << " aligned at " << memRequirements.alignment << std::endl;
        if (vkBindBufferMemory(device, tex.buffer, tex.memory, tex.bufferOffset) != VK_SUCCESS)
            std::cerr << "Faild to bind buffer memory.\n";
        tex.buffPtr = tex.ptr + tex.bufferOffset;
    }
}

void Vulkan::createCommands()
{
    int k = 0;
    frameFences.resize(4);
    // Query textures and frameFences
    for (auto &loader : Graphics::instance->getLoaders()) {
        auto tex0 = loader->getFrame(0);
        auto tex1 = loader->getFrame(1);
        auto tex2 = loader->getFrame(2);
        auto tex3 = loader->getFrame(3);
        frameFences[0].push_back(tex0->getFence());
        frameFences[1].push_back(tex1->getFence());
        frameFences[2].push_back(tex2->getFence());
        frameFences[3].push_back(tex3->getFence());
        texData[k].textures.push_back(tex0->getView());
        texData[k].textures.push_back(tex1->getView());
        texData[k].textures.push_back(tex2->getView());
        texData[k].textures.push_back(tex3->getView());
        ++k;
    }
    // Initialize descriptorSet
    createDescriptorSets();
    // Create draw commands
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicIndex;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création d'une command pool !");
    }
    submit.resize(4);
    acquired.resize(4);
    drawCommands.resize(4);
    drawn.resize(4);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 4;
    allocInfo.commandPool = pool;
    if (vkAllocateCommandBuffers(device, &allocInfo, drawCommands.data()) != VK_SUCCESS) {
        throw std::runtime_error("échec de l'allocation de command buffers!");
    }
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (int i = 0; i < 4; ++i) {
        // build submit ressources
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquired[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore for previously allocated command buffers.");
        }
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &drawn[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore for previously allocated command buffers.");
        }
        submit[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit[i].waitSemaphoreCount = 1;
        submit[i].pWaitSemaphores = &acquired[i];
        submit[i].pWaitDstStageMask = waitStages;
        submit[i].commandBufferCount = 1;
        submit[i].pCommandBuffers = &drawCommands[i];
        submit[i].signalSemaphoreCount = 1;
        submit[i].pSignalSemaphores = &drawn[i];
        // build draw command
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(drawCommands[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("erreur au début de l'enregistrement d'un command buffer!");
        }
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        vkCmdBeginRenderPass(drawCommands[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(drawCommands[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkDeviceSize offset = sizeof(UniformBlock);
        vkCmdBindVertexBuffers(drawCommands[i], 0, 1, &buffers, &offset);
        vkCmdBindDescriptorSets(drawCommands[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sets[i], 0, 0);
        vkCmdDraw(drawCommands[i], 4, 1, 0, 0);
        vkCmdEndRenderPass(drawCommands[i]);
        vkEndCommandBuffer(drawCommands[i]);
    }
}

void Vulkan::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize[2]{{},{}};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = 4;
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = 4*9;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSize;
    poolInfo.maxSets = 4;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("echec de la creation de la pool de descripteurs!");
    }
}

void Vulkan::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 4;
    VkDescriptorSetLayout descLayouts[]{descriptorSetLayout, descriptorSetLayout, descriptorSetLayout, descriptorSetLayout};
    allocInfo.pSetLayouts = descLayouts;

    sets.resize(4);
    if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("echec de l'allocation d'un set de descripteurs!");
    }
    VkDescriptorBufferInfo bufferInfo {buffers, 0, sizeof(UniformBlock)};
    for (size_t i = 0; i < 4; i++) {
        std::vector<VkWriteDescriptorSet> writeSet;
        writeSet.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sets[i], 2, 0, 1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bufferInfo, nullptr});

        VkDescriptorImageInfo imageInfo[8]{{},{},{},{},{},{},{},{}};
        for (int j = 0; j < 8; ++j) {
            imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[j].imageView = texData[j].textures[i];
        }

        writeSet.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sets[i], 1, 0, 8,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo, nullptr, nullptr});

        vkUpdateDescriptorSets(device, writeSet.size(), writeSet.data(), 0, nullptr);
    }
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    // for (const auto& availablePresentMode : availablePresentModes) {
    //     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         return availablePresentMode;
    //     }
    // }
    (void) availablePresentModes;

    return VK_PRESENT_MODE_FIFO_KHR; // We expect a stable frame order
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    // Real size is not always supported
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

bool Vulkan::isDeviceSuitable(VkPhysicalDevice pDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0)
        return false;

    bool extensionsSupported = checkDeviceExtensionSupport(pDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pDevice);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return extensionsSupported && swapChainAdequate;
}

SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice pDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtension.begin(), deviceExtension.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

void Vulkan::initDevice(const char *AppName, const char *EngineName, GLFWwindow *window, bool _hasLayer)
{
    hasLayer = _hasLayer;

    // initialize the vk::ApplicationInfo structure
    vk::ApplicationInfo applicationInfo( AppName, 1, EngineName, 1, VK_API_VERSION_1_1);

    // initialize the vk::InstanceCreateInfo
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, hasLayer ? validationLayers.size() : 0, validationLayers.data(), instanceExtension.size(), instanceExtension.data());

    if (hasLayer)
        initDebug(&instanceCreateInfo);

    // create a UniqueInstance
    instance = vk::createInstanceUnique(instanceCreateInfo);

    if (hasLayer)
        startDebug();

    initWindow(window);

    // get a physicalDevice
    for (const auto &pDevice : instance->enumeratePhysicalDevices()) {
        if (isDeviceSuitable(pDevice)) {
            physicalDevice = pDevice;
            if (pDevice.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                std::cout << "Integrated GPU found :)\n";
                break;
            }
        } else {
        }
    }
}

void Vulkan::initWindow(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(instance.get(), window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Vulkan::initQueues()
{
    // get the QueueFamilyProperties of PhysicalDevice
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // {TO CHECK} register all index into queueFamiliyProperties which supports graphics, present, or both
    size_t i = 0;
    std::vector<float> queuePriority(9, 0.0f);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pQueuePriorities = queuePriority.data();
    queueCreateInfo.pNext = nullptr;

    int maxGraphic = 0;
    int maxTransfer = 0;
    for (auto &qfp : queueFamilyProperties) {
        VkBool32 presentSupport = true;
        queueCreateInfo.queueCount = std::min(qfp.queueCount, 9u);
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (qfp.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (presentSupport) {
                graphicIndex = i;
                maxGraphic = queueCreateInfo.queueCount;
            } else {
                i++;
                continue;
            }
        } else if (qfp.queueFlags & vk::QueueFlagBits::eTransfer) {
            queueIndex = i;
            maxTransfer = queueCreateInfo.queueCount;
        } else {
            i++;
            continue;
        }
        queueCreateInfo.queueFamilyIndex = i;
        i++;
    }
    // Assume there is no dedicated transfer queue
    if (maxGraphic + maxTransfer < 9) {
        std::cerr << "Can't create enough queue, disable async mode.\n";
        std::cerr << "Graphics : " << maxGraphic << " | Transfer : " << maxTransfer << "\n";
        async = false;
        maxGraphic = 1;
        maxTransfer = 0;
        queueIndex = graphicIndex;
    } else if (maxGraphic == 9) {
        maxTransfer = 0;
        queueIndex = graphicIndex;
    } else if (maxTransfer >= 8) {
        maxTransfer = 8;
        maxGraphic = 1;
    } else {
        std::cerr << "Can't dispatch queues like expected, disable async mode.\n";
        std::cerr << "Graphics : " << maxGraphic << " | Transfer : " << maxTransfer << "\n";
        async = false;
        maxGraphic = 1;
        maxTransfer = 0;
        queueIndex = graphicIndex;
    }
    if (maxTransfer > 0) {
        queueCreateInfo.queueFamilyIndex = queueIndex;
        queueCreateInfo.queueCount = maxTransfer;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    queueCreateInfo.queueFamilyIndex = graphicIndex;
    queueCreateInfo.queueCount = maxGraphic;
    queueCreateInfos.push_back(queueCreateInfo);

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtension.size();
    createInfo.ppEnabledExtensionNames = deviceExtension.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    VkQueue tmpQueue;
    if (maxTransfer > 0) {
        for (int j = 0; j < maxTransfer; j++) {
            std::cout << "transferQueue\n";
            vkGetDeviceQueue(device, queueIndex, j, &tmpQueue);
            queues.push_back(tmpQueue);
        }
    }
    if (maxGraphic > 0) {
        for (int j = 0; j < maxGraphic; j++) {
            std::cout << "graphicQueue\n";
            vkGetDeviceQueue(device, graphicIndex, j, &tmpQueue);
            queues.push_back(tmpQueue);
        }
    }
    if (!async) {
        VkQueue uniqueQueue = queues.front();
        queues.clear();
        queues.resize(9);
        queues[8] = uniqueQueue;
    }
}

void Vulkan::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // the draw cover the whole surface
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // deepbuffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // deepbuffer
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments{colorAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création de la render pass!");
    }
}

void Vulkan::createFramebuffer()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("échec de la création d'un framebuffer!");
        }
    }
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("aucun type de memoire ne satisfait le buffer!");
}

VkCommandBuffer Vulkan::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = singlePool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (queue == VK_NULL_HANDLE)
        queue = queues[8];
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, singlePool, 1, &commandBuffer);
}

void Vulkan::setLayout(VkImage image, VkQueue queue)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    endSingleTimeCommands(commandBuffer, queue);
}

void Vulkan::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("échec de la creation d'un sampler!");
    }
}

void Vulkan::initSwapchain(int width, int height)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities, width, height);
    swapChainImageFormat = surfaceFormat.format;

    //uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    uint32_t imageCount = 4;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.imageArrayLayers = 1; // Pas besoin de 3D stéréoscopique
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfert nécessaire pour l'affichage
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Pas de transparence pour le contenu de la fenêtre
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_FALSE; // ne pas calculer les pixels masqués par une autre fenêtre
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création de la swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

VkImageView Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("échec de la creation de la vue sur une image!");
    }

    return imageView;
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkShaderModule Vulkan::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création d'un module shader!");
    }
    return shaderModule;
}

void Vulkan::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = &sampler;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkSampler samplers[8] {sampler, sampler, sampler, sampler, sampler, sampler, sampler, sampler};
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 1;
    textureBinding.descriptorCount = 8;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.pImmutableSamplers = samplers;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding uniformCollection{};
    uniformCollection.binding = 2;
    uniformCollection.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformCollection.descriptorCount = 1;
    uniformCollection.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    uniformCollection.pImmutableSamplers = nullptr; // Optionnel

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {samplerLayoutBinding, textureBinding, uniformCollection};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("echec de la creation d'un set de descripteurs!");
    }
}

void Vulkan::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shader/shader.vert.spv");
    auto fragShaderCode = readFile("shader/shader.frag.spv");

    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr; // define constant values if any

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.pSpecializationInfo = nullptr; // define constant values if any

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // Possibilité d'interrompre les liaisons entre les vertices pour les modes _STRIP
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{}; // Déformation de l'image
    viewport.width = swapChainExtent.height;
    viewport.height = viewport.width;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{}; // Découpage de l'image
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // Tout élément trop loin ou trop près est ramené à la couche la plus loin ou la plus proche
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // Désactiver la géométrie
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optionnel
    rasterizer.depthBiasClamp = 0.0f; // Optionnel
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optionnel

    VkPipelineMultisampleStateCreateInfo multisampling{}; // Pour l'anti-aliasing
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optionnel
    multisampling.pSampleMask = nullptr; // Optionnel
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optionnel
    multisampling.alphaToOneEnable = VK_FALSE; // Optionnel

    // frameBuffer Blend mode
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optionnel
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optionnel
    colorBlending.blendConstants[1] = 0.0f; // Optionnel
    colorBlending.blendConstants[2] = 0.0f; // Optionnel
    colorBlending.blendConstants[3] = 0.0f; // Optionnel

    VkDynamicState dynamicStates[] = {
        //VK_DYNAMIC_STATE_VIEWPORT,
        //VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 1;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optionnel
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optionnel

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création du pipeline layout!");
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optionnel
    depthStencil.maxDepthBounds = 1.0f; // Optionnel
    depthStencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optionnel
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optionnel
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    // Héritage - le changement entre 2 pipes ayant le même parent est plus rapide
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optionnel
    pipelineInfo.basePipelineIndex = -1; // Optionnel

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création de la pipeline graphique!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Vulkan::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}

// =============== DEBUG =============== //
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Vulkan::initDebug(vk::InstanceCreateInfo *instanceCreateInfo)
{
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr; // Optionnel

    instanceCreateInfo->setPNext(&debugCreateInfo);
}

void Vulkan::startDebug()
{
    std::vector<vk::LayerProperties>     layerProperties     = vk::enumerateInstanceLayerProperties();
    std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();

    if (CreateDebugUtilsMessengerEXT(instance.get(), &debugCreateInfo, nullptr, &callback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Vulkan::destroyDebug()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance.get(), "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance.get(), callback, nullptr);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void * /*pUserData*/)
{
    std::cerr << vk::to_string( static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ) ) << ": "
        << vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) ) << ":\n";
    std::cerr << "\t" << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    std::cerr << "\t" << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    std::cerr << "\t" << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        std::cerr << "\t" << "Queue Labels:\n";
        for (uint8_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            std::cerr << "\t\t" << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        std::cerr << "\t" << "CommandBuffer Labels:\n";
        for (uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            std::cerr << "\t\t" << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        std::cerr << "\t" << "Objects:\n";
        for ( uint8_t i = 0; i < pCallbackData->objectCount; i++ )
        {
            std::cerr << "\t\t" << "Object " << (int) i << "\n";
            std::cerr << "\t\t\t" << "objectType   = "
            << vk::to_string( static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType ) ) << "\n";
            std::cerr << "\t\t\t" << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                std::cerr << "\t\t\t" << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }
    return VK_TRUE;
}
