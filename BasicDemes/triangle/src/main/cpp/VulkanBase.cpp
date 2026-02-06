/*
 * Vulkan Example Base Class Implementation
 * Based on Sascha Willems' vulkanexamplebase
 */

#include "VulkanBase.hpp"
#include <algorithm>

VulkanExampleBase::~VulkanExampleBase() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);

        // Destroy synchronization primitives
        for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
            if (waitFences[i] != VK_NULL_HANDLE) {
                vkDestroyFence(device, waitFences[i], nullptr);
            }
            if (presentCompleteSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
            }
            if (renderCompleteSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
            }
        }

        // Destroy framebuffers
        for (auto& fb : frameBuffers) {
            vkDestroyFramebuffer(device, fb, nullptr);
        }

        // Destroy depth stencil
        if (depthStencil.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, depthStencil.view, nullptr);
        }
        if (depthStencil.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, depthStencil.image, nullptr);
        }
        if (depthStencil.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, depthStencil.memory, nullptr);
        }

        // Destroy render pass
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
        }

        // Destroy swapchain image views
        for (auto& buffer : swapChainBuffers) {
            vkDestroyImageView(device, buffer.view, nullptr);
        }

        // Destroy swapchain
        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }

        // Destroy command pool
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }

        // Destroy pipeline cache
        if (pipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(device, pipelineCache, nullptr);
        }

        // Destroy device
        vkDestroyDevice(device, nullptr);
    }

    // Destroy surface
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    // Destroy instance
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

void VulkanExampleBase::initVulkan(android_app* app) {
    androidApp = app;
    LOGI("Initializing Vulkan...");
    
    createInstance();
    createSurface();
    createDevice();
}

void VulkanExampleBase::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = title.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VulkanExample";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCI{};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appInfo;
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

    VK_CHECK_RESULT(vkCreateInstance(&instanceCI, nullptr, &instance));
    LOGI("Vulkan instance created");
}

void VulkanExampleBase::createSurface() {
    VkAndroidSurfaceCreateInfoKHR surfaceCI{};
    surfaceCI.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCI.window = androidApp->window;

    VK_CHECK_RESULT(vkCreateAndroidSurfaceKHR(instance, &surfaceCI, nullptr, &surface));
    LOGI("Android surface created");
}

void VulkanExampleBase::createDevice() {
    // Enumerate physical devices
    uint32_t gpuCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    assert(gpuCount > 0);

    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));
    physicalDevice = physicalDevices[0];

    // Get device properties
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

    LOGI("Using GPU: %s", deviceProperties.deviceName);

    // Find graphics queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }

    // Create logical device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCI{};
    queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCI.queueFamilyIndex = queueFamilyIndex;
    queueCI.queueCount = 1;
    queueCI.pQueuePriorities = &queuePriority;

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = 1;
    deviceCI.pQueueCreateInfos = &queueCI;
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    LOGI("Vulkan device created");
}

void VulkanExampleBase::createSwapChain() {
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    // Get surface formats
    uint32_t formatCount = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

    // Select format
    colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    colorSpace = surfaceFormats[0].colorSpace;
    for (const auto& format : surfaceFormats) {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM) {
            colorFormat = format.format;
            colorSpace = format.colorSpace;
            break;
        }
    }

    // Get present modes
    uint32_t presentModeCount = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

    VkExtent2D swapchainExtent = surfaceCaps.currentExtent;
    width = swapchainExtent.width;
    height = swapchainExtent.height;

    // Determine number of images
    uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && desiredImageCount > surfaceCaps.maxImageCount) {
        desiredImageCount = surfaceCaps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCI{};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = surface;
    swapchainCI.minImageCount = desiredImageCount;
    swapchainCI.imageFormat = colorFormat;
    swapchainCI.imageColorSpace = colorSpace;
    swapchainCI.imageExtent = swapchainExtent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.preTransform = surfaceCaps.currentTransform;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapchainCI.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCI.clipped = VK_TRUE;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

    // Get swapchain images
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
    std::vector<VkImage> images(imageCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

    // Create image views
    swapChainBuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        swapChainBuffers[i].image = images[i];

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image = images[i];
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = colorFormat;
        viewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr, &swapChainBuffers[i].view));
    }

    LOGI("Swapchain created: %dx%d, %d images", width, height, imageCount);
}

void VulkanExampleBase::createCommandPool() {
    VkCommandPoolCreateInfo cmdPoolCI{};
    cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.queueFamilyIndex = queueFamilyIndex;
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolCI, nullptr, &commandPool));
}

void VulkanExampleBase::createCommandBuffers() {
    VkCommandBufferAllocateInfo cmdBufAllocInfo{};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = commandPool;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandBufferCount = MAX_CONCURRENT_FRAMES;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocInfo, commandBuffers.data()));
}

void VulkanExampleBase::createSynchronizationPrimitives() {
    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));
    }
}

void VulkanExampleBase::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCI{};
    pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCI, nullptr, &pipelineCache));
}

void VulkanExampleBase::setupDepthStencil() {
    // Find supported depth format
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = format;
            break;
        }
    }

    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = { width, height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));
    VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

    VkImageViewCreateInfo viewCI{};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = depthFormat;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        viewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    viewCI.subresourceRange.baseMipLevel = 0;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.baseArrayLayer = 0;
    viewCI.subresourceRange.layerCount = 1;
    viewCI.image = depthStencil.image;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr, &depthStencil.view));
}

void VulkanExampleBase::setupRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments{};

    // Color attachment
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCI.pAttachments = attachments.data();
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassCI.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
}

void VulkanExampleBase::setupFrameBuffer() {
    frameBuffers.resize(imageCount);

    for (uint32_t i = 0; i < imageCount; i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainBuffers[i].view,
            depthStencil.view
        };

        VkFramebufferCreateInfo fbCI{};
        fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCI.renderPass = renderPass;
        fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbCI.pAttachments = attachments.data();
        fbCI.width = width;
        fbCI.height = height;
        fbCI.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbCI, nullptr, &frameBuffers[i]));
    }
}

void VulkanExampleBase::prepare() {
    createSwapChain();
    createCommandPool();
    createCommandBuffers();
    createSynchronizationPrimitives();
    createPipelineCache();
    setupDepthStencil();
    setupRenderPass();
    setupFrameBuffer();
    prepared = true;
    LOGI("Vulkan preparation complete");
}

void VulkanExampleBase::cleanup() {
    prepared = false;
}

void VulkanExampleBase::prepareFrame() {
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
        presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &currentBuffer);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle resize
        LOGW("Swapchain out of date or suboptimal");
    }
}

void VulkanExampleBase::submitFrame() {
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &currentBuffer;

    vkQueuePresentKHR(queue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
}

uint32_t VulkanExampleBase::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    LOGE("Could not find suitable memory type!");
    return 0;
}

VkShaderModule VulkanExampleBase::loadShader(const std::string& filename) {
    LOGI("Loading shader: %s", filename.c_str());

    AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("FATAL: Could not open shader file: %s", filename.c_str());
        LOGE("Make sure shader files are compiled and placed in assets/shaders/");
        return VK_NULL_HANDLE;
    }

    size_t size = AAsset_getLength(asset);
    LOGI("Shader file size: %zu bytes", size);

    char* code = new char[size];
    AAsset_read(asset, code, size);
    AAsset_close(asset);

    VkShaderModuleCreateInfo shaderModuleCI{};
    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.codeSize = size;
    shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(code);

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

    delete[] code;
    LOGI("Shader loaded successfully: %s", filename.c_str());
    return shaderModule;
}

void VulkanExampleBase::setImageLayout(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;

    switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
    }

    switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;
        default:
            break;
    }

    vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanExampleBase::renderLoop() {
    int events;
    android_poll_source* source;

    // Use the same do-while pattern as the reference app:
    // - Poll ONE event per iteration (not drain all events)
    // - When not prepared: block with -1 to wait for APP_CMD_INIT_WINDOW
    // - When prepared: use 0 for non-blocking so we can render immediately
    do {
        if (ALooper_pollOnce(prepared ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(source->app, source);
            }
        }

        // Process GameActivity input events
        if (androidApp != nullptr) {
            android_input_buffer* inputBuffer = android_app_swap_input_buffers(androidApp);
            if (inputBuffer != nullptr) {
                if (inputBuffer->motionEventsCount > 0) {
                    android_app_clear_motion_events(inputBuffer);
                }
                if (inputBuffer->keyEventsCount > 0) {
                    android_app_clear_key_events(inputBuffer);
                }
            }
        }

        // Render frame if ready
        if (prepared && !paused) {
            render();
        }
    } while (androidApp == nullptr || androidApp->destroyRequested == 0);

    LOGI("Exiting render loop");
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
    cleanup();
}

// Static callback handler
void VulkanExampleBase::handleAppCommand(android_app* app, int32_t cmd) {
    VulkanExampleBase* example = reinterpret_cast<VulkanExampleBase*>(app->userData);
    if (example) {
        // Ensure androidApp is set
        if (example->androidApp == nullptr) {
            example->androidApp = app;
        }
        example->handleAppCommandInternal(cmd);
    }
}

void VulkanExampleBase::handleAppCommandInternal(int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW received");
            if (androidApp->window != nullptr) {
                initVulkan(androidApp);
                prepare();
                LOGI("Vulkan initialized and prepared, ready to render");
            } else {
                LOGW("APP_CMD_INIT_WINDOW: window is null!");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW received");
            cleanup();
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS");
            paused = false;
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS");
            paused = true;
            break;
        default:
            LOGD("Unhandled app command: %d", cmd);
            break;
    }
}
