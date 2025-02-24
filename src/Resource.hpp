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

void transitionImageLayout(const Context &ctx, int imageCount, const VkImage *image, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuffer);

void copyBufferToImage(const Context &ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


class DescriptorSet {
public:
    VkDescriptorSet set;
    VkDescriptorSetLayout layout;

    DescriptorSet() : set(nullptr), layout(nullptr) {};

    void initialise(const Context &ctx);

    void addBinding(const VkDescriptorSetLayoutBinding &bindingLayout);

    void addVertexBufferWrite(VkBuffer buffer, VkDeviceSize bufferRange, uint32_t binding);

    void addSSBOWrite(VkBuffer buffer, VkDeviceSize bufferRange, uint32_t binding);

    void addImageWrite(VkImageView imageView, uint32_t binding);

    void addSamplerWrite(VkImageView imageView, VkSampler sampler, uint32_t binding);

    void destroy(const Context &ctx) const;

private:
    std::vector<VkDescriptorSetLayoutBinding*> bindings;
    std::vector<VkWriteDescriptorSet*> writeSets;
};

class Resource {
public:
    enum {
        Buffer,
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
            uint32_t samplerBinding;
        } image;
    } data;


    Resource() : type(), descriptorSet(VK_NULL_HANDLE),
                 ctx(nullptr),
                 binding(0), data() {};

    Resource(
        Context &ctx,
        uint32_t binding,
        DescriptorSet *descriptorSet,
        int bufferSize,
        const void* bufferData,
        bool isSSBO,
        bool isVertexShaderAccessible,
        bool isTransferSource
    );

    Resource(
        Context &ctx,
        uint32_t imageBinding,
        uint32_t samplerBinding,
        DescriptorSet *descriptorSet
    );

    void destroy() const;

private:
    void updateDescriptorSet() const;
};

#endif //RESOURCE_H
