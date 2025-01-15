#include "Resource.h"

#include <memory>
#include <stdexcept>

uint32_t findMemoryType(
    const Context &ctx,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void createBuffer(
    const Context &ctx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(ctx,memRequirements.memoryTypeBits,properties);

    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(ctx.device, buffer, bufferMemory, 0);
}

void copyBuffer(
    const Context &ctx,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size
) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);

    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

Resource::Resource(
    const Context &ctx,
    uint32_t binding,
    VkDescriptorType descriptorType,
    VkDescriptorType descriptorStageFlags,
    VkDescriptorPool descriptorPool,
    int bufferSize,
    const void* bufferData,
    bool isVertexShaderAccessible
) {
    this->binding = binding;
    this->ctx = ctx;

    // descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = descriptorStageFlags;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }

    // descriptor sets
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(ctx.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    // buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(ctx.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, bufferData, bufferSize);
    vkUnmapMemory(ctx.device, stagingBufferMemory);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT // used as a storage buffer for compute shader
                             | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // transfer data from host to GPU
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (isVertexShaderAccessible) {
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;  // used as a vertex buffer for vert shader
    }

    createBuffer(ctx, bufferSize, usage, properties, buffer, bufferMemory);
    // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
    copyBuffer(ctx, stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(ctx.device, stagingBuffer, nullptr);
    vkFreeMemory(ctx.device, stagingBufferMemory, nullptr);
}

void Resource::updateDescriptorSets(VkBuffer otherBuffer, VkDeviceSize otherBufferRange) {
    // update descriptor sets
    // TODO this only really makes sense for SSBO storage
    VkDescriptorBufferInfo storageBufferInfoLast{};
    // storageBufferInfoLastFrame.buffer = storageBuffers[(i-1) % MAX_FRAMES_IN_FLIGHT];
    storageBufferInfoLast.buffer = otherBuffer;
    storageBufferInfoLast.offset = 0;
    storageBufferInfoLast.range = otherBufferRange;

    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &storageBufferInfoLast;
    descriptorWrite.pNext = nullptr;

    vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
}
