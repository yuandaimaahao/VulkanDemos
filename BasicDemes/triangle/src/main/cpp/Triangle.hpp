/*
 * Vulkan Triangle Example
 * Based on Sascha Willems' triangle example
 */

#pragma once

#include "VulkanBase.hpp"
#include <array>

class Triangle : public VulkanExampleBase {
public:
    // Vertex structure with position and color
    struct Vertex {
        float position[3];
        float color[3];
    };

    // Buffer structure for Vulkan buffers
    struct VulkanBuffer {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer handle = VK_NULL_HANDLE;
    };

    // Uniform buffer for shader data
    struct UniformBuffer : VulkanBuffer {
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        uint8_t* mapped = nullptr;
    };

    // Shader data passed to vertex shader
    struct ShaderData {
        float projectionMatrix[16];
        float modelMatrix[16];
        float viewMatrix[16];
    };

private:
    // Vertex and index buffers
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    uint32_t indexCount = 0;

    // Uniform buffers (one per frame in flight)
    std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

    // Descriptor set layout and pool
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Pipeline layout and pipeline
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    // Rotation angle for animation
    float rotation = 0.0f;

public:
    Triangle();
    ~Triangle() override;

protected:
    void prepare() override;
    void render() override;
    void cleanup() override;

private:
    // Setup methods
    void createVertexBuffer();
    void createUniformBuffers();
    void createDescriptors();
    void createPipeline();

    // Update uniform buffer for current frame
    void updateUniformBuffer();

    // Matrix helper functions
    void createIdentityMatrix(float* matrix);
    void createPerspectiveMatrix(float* matrix, float fov, float aspect, float near, float far);
    void createLookAtMatrix(float* matrix, float eyeX, float eyeY, float eyeZ,
                            float centerX, float centerY, float centerZ,
                            float upX, float upY, float upZ);
    void createRotationMatrix(float* matrix, float angle, float x, float y, float z);
};
