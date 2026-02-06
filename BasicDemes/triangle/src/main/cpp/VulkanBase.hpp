/*
 * Vulkan Example Base Class (Simplified version for Android)
 * Based on Sascha Willems' vulkanexamplebase
 *
 * Copyright (C) 2024 - Simplified for Android GameActivity
 */

#pragma once

#include <vulkan/vulkan.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>

#include <vector>
#include <array>
#include <string>
#include <cassert>
#include <cstring>

// Logging macros
#define LOG_TAG "VulkanExample"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Vulkan check macro
#define VK_CHECK_RESULT(f) \
    do { \
        VkResult res = (f); \
        if (res != VK_SUCCESS) { \
            LOGE("Vulkan error %d at %s:%d", res, __FILE__, __LINE__); \
            assert(res == VK_SUCCESS); \
        } \
    } while(0)

// Maximum number of concurrent frames
constexpr uint32_t MAX_CONCURRENT_FRAMES = 2;

/**
 * @brief Vulkan Example Base Class
 * 
 * Provides common Vulkan initialization and rendering infrastructure.
 * Derived classes should override prepare() and render() methods.
 */
class VulkanExampleBase {
public:
    // Structures
    struct SwapChainBuffer {
        VkImage image;
        VkImageView view;
    };

    struct DepthStencil {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    };

protected:
    // Android app context
    android_app* androidApp = nullptr;

    // Vulkan instance and device
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    // Physical device properties
    VkPhysicalDeviceProperties deviceProperties{};
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

    // Surface and swapchain
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace;
    std::vector<SwapChainBuffer> swapChainBuffers;
    uint32_t imageCount = 0;

    // Depth stencil
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    DepthStencil depthStencil{};

    // Render pass and framebuffers
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> frameBuffers;

    // Command pool and buffers
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};

    // Synchronization
    std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{};
    std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores{};
    std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

    // Pipeline cache
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

    // State
    bool prepared = false;
    bool paused = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t currentFrame = 0;
    uint32_t currentBuffer = 0;

    // Settings
    std::string title = "Vulkan Example";
    VkClearColorValue defaultClearColor = {{ 0.025f, 0.025f, 0.025f, 1.0f }};

public:
    VulkanExampleBase() = default;
    virtual ~VulkanExampleBase();

    // Initialize Vulkan
    void initVulkan(android_app* app);
    
    // Main render loop
    void renderLoop();

    // Static callback for Android app commands
    // Note: GameActivity does not use onInputEvent callback
    static void handleAppCommand(android_app* app, int32_t cmd);

protected:
    // Virtual methods to be overridden by derived classes
    virtual void prepare();
    virtual void render() = 0;
    virtual void cleanup();

    // Setup methods
    virtual void setupRenderPass();
    virtual void setupDepthStencil();
    virtual void setupFrameBuffer();

    // Helper methods
    void createInstance();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronizationPrimitives();
    void createPipelineCache();
    void createDepthStencil();
    void createRenderPass();
    void createFrameBuffers();

    // Utility methods
    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
    VkShaderModule loadShader(const std::string& filename);
    void setImageLayout(
        VkCommandBuffer cmdBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    // Frame handling
    void prepareFrame();
    void submitFrame();

private:
    void handleAppCommandInternal(int32_t cmd);
};
