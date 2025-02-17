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

void transitionImageLayout(const Context &ctx, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

void copyBufferToImage(const Context &ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


class DescriptorSet {
public:
    VkDescriptorSet set;

    DescriptorSet() : set(nullptr) {};

    DescriptorSet(const Context &ctx, uint32_t descriptorSetCount);

    void addSSBOWrite(VkBuffer buffer, VkDeviceSize bufferRange, uint32_t binding);

    void addImageWrite(VkImageView imageView, VkSampler sampler, uint32_t binding);

    void updateDescriptorSet(const Context &ctx);

private:
    std::vector<VkWriteDescriptorSet> writeSets;
};

class Resource {
public:
    enum {
        SSBO,
        Image
    } type;
    DescriptorSet* descriptorSet;
    Context* ctx;
    uint32_t binding;
    union {
        struct {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDeviceSize size;
        } buffer;
        struct {
            VkImage image;
            VkDeviceMemory memory;
            VkImageView view;
            VkSampler sampler;
        } image;
    } data;


    Resource() : type(), descriptorSet(VK_NULL_HANDLE),
                 ctx(nullptr),
                 binding(0), data() {};

    Resource(
        const Context &ctx,
        uint32_t binding,
        DescriptorSet *descriptorSet,
        int bufferSize,
        const void* bufferData,
        bool isVertexShaderAccessible,
        bool memoryIsHostVisible,
        bool isTransferSource
    );

    Resource(
        const Context &ctx,
        uint32_t binding,
        DescriptorSet *descriptorSet
    );

    void updateDescriptorSet();

    void destroy() const;
};

#endif //RESOURCE_H
