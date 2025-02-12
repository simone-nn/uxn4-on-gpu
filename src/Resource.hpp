#ifndef RESOURCE_H
#define RESOURCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "DeviceController.hpp"

uint32_t findMemoryType(
    const Context &ctx,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
);

void createBuffer(
    const Context &ctx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
);

void copyBuffer(
    const Context &ctx,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size
);

class Resource {
    public:
    VkDescriptorSet descriptorSet;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    uint32_t binding;
    Context* ctx;

    Resource() : descriptorSet(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE), bufferMemory(VK_NULL_HANDLE), binding(0), ctx(nullptr) {};

    Resource(
        const Context &ctx,
        uint32_t binding,
        int bufferSize,
        const void* bufferData,
        bool isVertexShaderAccessible,
        bool memoryIsHostVisible,
        bool isTransferSource
    );

    void updateDescriptorSets(VkBuffer otherBuffer, VkDeviceSize otherBufferRange) const;

    void destroy() const;
};

#endif //RESOURCE_H
