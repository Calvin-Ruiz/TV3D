/*
** EPITECH PROJECT, 2020
** TV3D
** File description:
** Vulkan.hpp
*/

#ifndef VULKAN_HPP_
#define VULKAN_HPP_

#include <iostream>
#include <string>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "vecmath.hpp"
#include <mutex>
#include <atomic>

class GLFWwindow;

extern int gwidth;
extern int gheight;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBlock {
    Vec2f position;
    float displayHeight;
    VkBool32 hasAlpha;
    float brightPos;
};

struct TexData {
    std::vector<VkImageView> textures; // Textures
    VkDeviceMemory memory; // textures memory
    VkDeviceSize size; // texture size (including offset)
    void *ptr; // Pointer to texture memory
    VkBuffer buffer; // Buffer used to dump texture in optimal layout format
    VkDeviceSize bufferOffset; // offset of buffer in .memory
    void *buffPtr; // Pointer to buffer memory
};

class Vulkan {
public:
    Vulkan(int width, int height, GLFWwindow *window, bool debug);
    virtual ~Vulkan();
    Vulkan(const Vulkan &cpy) = default;
    Vulkan &operator=(const Vulkan &src) = default;

    void setLayout(VkImage image, VkQueue queue);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void waitFrame();
    void drawFrame();
    UniformBlock *getUniform() {return (UniformBlock *) buffPtr;}
    TexData &getDataBlock(int idx) {return texData[idx];}
    void setDataSize(int idx, int size) {texData[idx].size = size;}
    VkQueue getQueue(int idx) {return queues[idx];}
    uint32_t getQueueIndex() {return queueIndex;}
    // Call this once all textures were initialized
    void createCommands();
    bool pushQueue();
    inline void addSubmit(VkSubmitInfo *submitInfo, VkFence fence)
    {
        unsigned char pos = writePos++;
        syncSubmitQueue[pos] = submitInfo;
        syncFenceQueue[pos] = fence;
        ++queueSize;
    }

    const VkDevice &refDevice;
    const VkRenderPass &refRenderPass;
    const std::vector<VkFramebuffer> &refSwapChainFramebuffers;
private:
    uint32_t frameID = 3;
    // initialize all buffers
    void initDevice(const char *AppName, const char *EngineName, GLFWwindow *window, bool _hasLayer = false);
    void initQueues();
    vk::UniqueInstance instance;
    vk::PhysicalDevice physicalDevice;
    void initWindow(GLFWwindow *window);
    VkSurfaceKHR surface;
    VkDevice device;
    uint32_t graphicIndex = -1;
    uint32_t queueIndex = -1; // transfer
    std::vector<VkQueue> queues;
    void initSwapchain(int width, int height);
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void createImageViews();
    std::vector<VkImageView> swapChainImageViews;
    void createDescriptorSetLayout();
    VkDescriptorSetLayout descriptorSetLayout;
    void createRenderPass();
    VkRenderPass renderPass;
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char> &code);
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    void createFramebuffer();
    std::vector<VkFramebuffer> swapChainFramebuffers;
    void createDescriptorPool();
    void createDescriptorSets();

    // initCommands();
    VkPipelineStageFlags waitStages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::vector<VkSubmitInfo> submit;
    std::vector<VkSemaphore> acquired;
    std::vector<VkCommandBuffer> drawCommands;
    std::vector<VkSemaphore> drawn;

    void initBuffers();
    VkCommandPool singlePool;
    VkCommandPool pool;
    void *buffPtr = nullptr;
    VkDeviceMemory bufferMemory;
    VkBuffer buffers;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> sets;
    std::vector<TexData> texData;
    std::vector<std::vector<VkFence>> frameFences; // Frame fences

    // Texture-related datas
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);
    void createTextureSampler();
    VkSampler sampler;

    void initDebug(vk::InstanceCreateInfo *instanceCreateInfo);
    void startDebug();
    void destroyDebug();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void * /*pUserData*/);
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    VkDebugUtilsMessengerEXT callback;

    bool checkDeviceExtensionSupport(VkPhysicalDevice pDevice);
    bool isDeviceSuitable(VkPhysicalDevice pDevice);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    std::vector<const char *> instanceExtension = {"VK_KHR_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    std::vector<const char *> deviceExtension = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    bool hasLayer;

    // submission mutex
    bool async = true;
    std::vector<VkSubmitInfo *> syncSubmitQueue;
    std::vector<VkFence> syncFenceQueue;
    std::vector<VkSubmitInfo *> prioritizedSubmitQueue;
    unsigned char prioritizedReadPos = 0;
    unsigned char readPos = 0;
    unsigned char prioritizedWritePos = 0;
    std::atomic<unsigned char> writePos;
    std::atomic<short> queueSize;
    std::atomic<char> prioritizedQueueSize;
};

#endif /* VULKAN_HPP_ */
