#ifndef RESOURCE_H
#define RESOURCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "main.h"

namespace resource {
    uint32_t findMemoryType(
        Context &ctx,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    );

    void createBuffer(
        Context &ctx,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    );

    void copyBuffer(
        Context &ctx,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
    );
}

class Resource {
    public:
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    uint32_t binding;
    Context ctx;

    Resource(
        const Context &ctx,
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkDescriptorType descriptorStageFlags,
        VkDescriptorPool descriptorPool,
        int bufferSize,
        const void* bufferData,
        bool isVertexShaderAccessible
    );

    void updateDescriptorSets(VkBuffer otherBuffer, VkDeviceSize otherBufferRange);
};

#endif //RESOURCE_H
