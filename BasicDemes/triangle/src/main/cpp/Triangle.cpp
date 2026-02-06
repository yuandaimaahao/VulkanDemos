/*
 * Vulkan Triangle Example Implementation
 * Based on Sascha Willems' triangle example
 */

#include "Triangle.hpp"
#include <cmath>

Triangle::Triangle() : VulkanExampleBase() {
    title = "Vulkan Triangle";
    defaultClearColor = {{ 0.0f, 0.34f, 0.90f, 1.0f }};
}

Triangle::~Triangle() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);

        // Destroy pipeline
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }

        // Destroy descriptor resources
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }

        // Destroy uniform buffers
        for (auto& ub : uniformBuffers) {
            if (ub.handle != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, ub.handle, nullptr);
            }
            if (ub.memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, ub.memory, nullptr);
            }
        }

        // Destroy vertex/index buffers
        if (vertexBuffer.handle != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer.handle, nullptr);
        }
        if (vertexBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, vertexBuffer.memory, nullptr);
        }
        if (indexBuffer.handle != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, indexBuffer.handle, nullptr);
        }
        if (indexBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, indexBuffer.memory, nullptr);
        }
    }
}

void Triangle::prepare() {
    VulkanExampleBase::prepare();
    createVertexBuffer();
    createUniformBuffers();
    createDescriptors();
    createPipeline();
    LOGI("Triangle preparation complete");
}

void Triangle::cleanup() {
    VulkanExampleBase::cleanup();
}

void Triangle::createVertexBuffer() {
    // Define triangle vertices (position and color)
    std::vector<Vertex> vertices = {
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // Red
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Green
        {{ 0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}   // Blue
    };
    uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));

    // Define indices
    std::vector<uint32_t> indices = {0, 1, 2};
    indexCount = static_cast<uint32_t>(indices.size());
    uint32_t indexBufferSize = indexCount * sizeof(uint32_t);

    // Create staging buffer
    VulkanBuffer stagingBuffer;
    VkBufferCreateInfo stagingBufferCI{};
    stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferCI.size = vertexBufferSize + indexBufferSize;
    stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VK_CHECK_RESULT(vkCreateBuffer(device, &stagingBufferCI, nullptr, &stagingBuffer.handle));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, stagingBuffer.handle, &memReqs);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffer.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer.handle, stagingBuffer.memory, 0));

    // Copy data to staging buffer
    uint8_t* data;
    VK_CHECK_RESULT(vkMapMemory(device, stagingBuffer.memory, 0, memAlloc.allocationSize, 0, (void**)&data));
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy(data + vertexBufferSize, indices.data(), indexBufferSize);
    vkUnmapMemory(device, stagingBuffer.memory);

    // Create device local vertex buffer
    VkBufferCreateInfo vertexBufferCI{};
    vertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferCI.size = vertexBufferSize;
    vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCI, nullptr, &vertexBuffer.handle));
    vkGetBufferMemoryRequirements(device, vertexBuffer.handle, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertexBuffer.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, vertexBuffer.handle, vertexBuffer.memory, 0));

    // Create device local index buffer
    VkBufferCreateInfo indexBufferCI{};
    indexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferCI.size = indexBufferSize;
    indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VK_CHECK_RESULT(vkCreateBuffer(device, &indexBufferCI, nullptr, &indexBuffer.handle));
    vkGetBufferMemoryRequirements(device, indexBuffer.handle, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indexBuffer.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, indexBuffer.handle, indexBuffer.memory, 0));

    // Copy from staging buffer to device local buffers
    VkCommandBuffer copyCmd;
    VkCommandBufferAllocateInfo cmdBufAllocInfo{};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = commandPool;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocInfo, &copyCmd));

    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufBeginInfo));

    VkBufferCopy copyRegion{};
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, vertexBuffer.handle, 1, &copyRegion);

    copyRegion.size = indexBufferSize;
    copyRegion.srcOffset = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, indexBuffer.handle, 1, &copyRegion);

    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

    // Clean up staging buffer
    vkDestroyBuffer(device, stagingBuffer.handle, nullptr);
    vkFreeMemory(device, stagingBuffer.memory, nullptr);

    LOGI("Vertex buffer created");
}

void Triangle::createUniformBuffers() {
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = sizeof(ShaderData);
    bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCI, nullptr, &uniformBuffers[i].handle));

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, uniformBuffers[i].handle, &memReqs);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &uniformBuffers[i].memory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].handle, uniformBuffers[i].memory, 0));
        VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, 
            (void**)&uniformBuffers[i].mapped));
    }

    LOGI("Uniform buffers created");
}

void Triangle::createDescriptors() {
    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_CONCURRENT_FRAMES;

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes = &poolSize;
    poolCI.maxSets = MAX_CONCURRENT_FRAMES;

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolCI, nullptr, &descriptorPool));

    // Create descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 1;
    layoutCI.pBindings = &layoutBinding;

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &descriptorSetLayout));

    // Allocate and update descriptor sets
    for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].handle;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ShaderData);

        VkWriteDescriptorSet writeDS{};
        writeDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDS.dstSet = uniformBuffers[i].descriptorSet;
        writeDS.dstBinding = 0;
        writeDS.descriptorCount = 1;
        writeDS.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDS.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &writeDS, 0, nullptr);
    }

    LOGI("Descriptors created");
}

void Triangle::createPipeline() {
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

    // Load shaders
    VkShaderModule vertShaderModule = loadShader("shaders/triangle.vert.spv");
    VkShaderModule fragShaderModule = loadShader("shaders/triangle.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    // Vertex input
    VkVertexInputBindingDescription vertexInputBinding{};
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(Vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes{};
    // Position
    vertexInputAttributes[0].binding = 0;
    vertexInputAttributes[0].location = 0;
    vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[0].offset = offsetof(Vertex, position);
    // Color
    vertexInputAttributes[1].binding = 0;
    vertexInputAttributes[1].location = 1;
    vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributes[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
    inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
    rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
    rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCI.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
    depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCI.depthTestEnable = VK_TRUE;
    depthStencilStateCI.depthWriteEnable = VK_TRUE;
    depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCI.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = 0xF;
    blendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments = &blendAttachmentState;

    // Dynamic states
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCI.pDynamicStates = dynamicStates.data();

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;
    pipelineCI.layout = pipelineLayout;
    pipelineCI.renderPass = renderPass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

    // Cleanup shader modules
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    LOGI("Pipeline created");
}

void Triangle::updateUniformBuffer() {
    ShaderData shaderData{};

    // Create perspective matrix (Vulkan clip space: Y is flipped, depth 0..1)
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    createPerspectiveMatrix(shaderData.projectionMatrix, 60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 256.0f);

    // Flip Y for Vulkan (Vulkan Y axis is inverted compared to OpenGL)
    shaderData.projectionMatrix[5] *= -1.0f;

    // Create view matrix (look at origin from z=2.5, looking towards negative z)
    createLookAtMatrix(shaderData.viewMatrix, 0.0f, 0.0f, 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // Create model matrix with rotation
    createRotationMatrix(shaderData.modelMatrix, rotation, 0.0f, 0.0f, 1.0f);

    // Copy to uniform buffer
    memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));
}

void Triangle::render() {
    static bool firstFrame = true;
    if (firstFrame) {
        LOGI("Triangle::render() - first frame! width=%u height=%u", width, height);
        firstFrame = false;
    }

    prepareFrame();

    // Update rotation
    rotation += 0.5f;
    if (rotation > 360.0f) {
        rotation -= 360.0f;
    }

    updateUniformBuffer();

    // Build command buffer
    VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
    VK_CHECK_RESULT(vkResetCommandBuffer(cmdBuffer, 0));

    VkCommandBufferBeginInfo cmdBufBeginInfo{};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo));

    // Begin render pass
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = defaultClearColor;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffers[currentBuffer];
    renderPassBeginInfo.renderArea.extent = {width, height};
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = {width, height};
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // Bind pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Bind descriptor set
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 
        0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);

    // Bind vertex buffer
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.handle, offsets);

    // Bind index buffer
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed triangle
    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

    submitFrame();
}

// Matrix helper functions
void Triangle::createIdentityMatrix(float* matrix) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void Triangle::createPerspectiveMatrix(float* matrix, float fov, float aspect, float near, float far) {
    memset(matrix, 0, 16 * sizeof(float));
    float tanHalfFov = tanf(fov / 2.0f);
    
    matrix[0] = 1.0f / (aspect * tanHalfFov);
    matrix[5] = 1.0f / tanHalfFov;
    matrix[10] = -(far + near) / (far - near);
    matrix[11] = -1.0f;
    matrix[14] = -(2.0f * far * near) / (far - near);
}

void Triangle::createLookAtMatrix(float* matrix, float eyeX, float eyeY, float eyeZ,
                                  float centerX, float centerY, float centerZ,
                                  float upX, float upY, float upZ) {
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    float fLen = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= fLen; fy /= fLen; fz /= fLen;

    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    float sLen = sqrtf(sx * sx + sy * sy + sz * sz);
    sx /= sLen; sy /= sLen; sz /= sLen;

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    matrix[0] = sx;  matrix[4] = sy;  matrix[8]  = sz;  matrix[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
    matrix[1] = ux;  matrix[5] = uy;  matrix[9]  = uz;  matrix[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    matrix[2] = -fx; matrix[6] = -fy; matrix[10] = -fz; matrix[14] = (fx * eyeX + fy * eyeY + fz * eyeZ);
    matrix[3] = 0.0f; matrix[7] = 0.0f; matrix[11] = 0.0f; matrix[15] = 1.0f;
}

void Triangle::createRotationMatrix(float* matrix, float angle, float x, float y, float z) {
    float rad = angle * 3.14159265f / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);
    float len = sqrtf(x * x + y * y + z * z);
    x /= len; y /= len; z /= len;

    matrix[0] = x * x * (1 - c) + c;
    matrix[1] = y * x * (1 - c) + z * s;
    matrix[2] = x * z * (1 - c) - y * s;
    matrix[3] = 0.0f;

    matrix[4] = x * y * (1 - c) - z * s;
    matrix[5] = y * y * (1 - c) + c;
    matrix[6] = y * z * (1 - c) + x * s;
    matrix[7] = 0.0f;

    matrix[8] = x * z * (1 - c) + y * s;
    matrix[9] = y * z * (1 - c) - x * s;
    matrix[10] = z * z * (1 - c) + c;
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}
