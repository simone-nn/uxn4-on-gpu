#include "Resource.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ctx);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(ctx, commandBuffer);
}

void transitionImageLayout(
    const Context &ctx,
    int imageCount, const VkImage* image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkCommandBuffer cmdBuffer
) {
    bool singleTimeBuffer = cmdBuffer == VK_NULL_HANDLE;
    if (singleTimeBuffer) {
        cmdBuffer = beginSingleTimeCommands(ctx);
    }

    for (int i = 0; i < imageCount; ++i) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image[i];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        } else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;  // Compute shader writes
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;   // Fragment shader reads
            sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            cmdBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    if (singleTimeBuffer) {
        endSingleTimeCommands(ctx, cmdBuffer);
    }
}

void copyBufferToImage(const Context &ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ctx);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(ctx, commandBuffer);
}


// -- Descriptor Set Wrapper --
void DescriptorSet::initialise(const Context &ctx) {
    // Descriptor Layout
    std::vector<VkDescriptorSetLayoutBinding> b;
    b.reserve(bindings.size());
    for (auto & binding : bindings) {
        b.emplace_back(*binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = b.size();
    layoutInfo.pBindings = b.data();
    if (vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = ctx.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(ctx.device, &allocInfo, &set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    // Descriptor Writes
    std::vector<VkWriteDescriptorSet> writes;
    for (auto & writeSet : writeSets) {
        // Before updating the descriptor set, we need to update the write sets to reference the new descriptor set
        writeSet->dstSet = set;
        // Deref the write sets
        writes.emplace_back(*writeSet);
    }
    vkUpdateDescriptorSets(
        ctx.device,
        writes.size(),
        writes.data(),
        0,
        nullptr);
}

void DescriptorSet::addSSBOWrite(VkBuffer buffer, VkDeviceSize bufferRange, uint32_t binding) {
    auto* storageBufferInfo = new VkDescriptorBufferInfo;
    storageBufferInfo->buffer = buffer;
    storageBufferInfo->offset = 0;
    storageBufferInfo->range = bufferRange;

    auto* descriptorWrite = new VkWriteDescriptorSet;
    descriptorWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite->dstSet = VK_NULL_HANDLE; // this will be updated once the set is initialised
    descriptorWrite->dstBinding = binding;
    descriptorWrite->dstArrayElement = 0;
    descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite->descriptorCount = 1;
    descriptorWrite->pBufferInfo = storageBufferInfo;
    descriptorWrite->pNext = nullptr;

    writeSets.emplace_back(descriptorWrite);
}

void DescriptorSet::addImageWrite(VkImageView imageView, uint32_t binding) {
    auto* imageInfo = new VkDescriptorImageInfo;
    imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo->imageView = imageView;

    auto* descriptorWrite = new VkWriteDescriptorSet;
    descriptorWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite->dstSet = VK_NULL_HANDLE; // this will be updated once the set is initialised
    descriptorWrite->dstBinding = binding;
    descriptorWrite->dstArrayElement = 0;
    descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite->descriptorCount = 1;
    descriptorWrite->pImageInfo = imageInfo;
    descriptorWrite->pNext = nullptr;

    writeSets.emplace_back(descriptorWrite);
}

void DescriptorSet::addSamplerWrite(VkImageView imageView, VkSampler sampler, uint32_t binding) {
    auto* imageInfo = new VkDescriptorImageInfo;
    imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo->imageView = imageView;
    imageInfo->sampler = sampler;

    auto* descriptorWrite = new VkWriteDescriptorSet;
    descriptorWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite->dstSet = VK_NULL_HANDLE; // this will be updated once the set is initialised
    descriptorWrite->dstBinding = binding;
    descriptorWrite->dstArrayElement = 0;
    descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite->descriptorCount = 1;
    descriptorWrite->pImageInfo = imageInfo;
    descriptorWrite->pNext = nullptr;

    writeSets.emplace_back(descriptorWrite);
}

void DescriptorSet::destroy(const Context &ctx) const {
    vkDestroyDescriptorSetLayout(ctx.device, layout, nullptr);
}


// -- Resource --
Resource::Resource(
    Context &ctx,
    uint32_t binding,
    DescriptorSet *descriptorSet,
    int bufferSize,
    const void* bufferData,
    bool isVertexShaderAccessible,
    bool isTransferSource
) {
    this->type = SSBO;
    this->binding = binding;
    this->ctx = &ctx;
    this->descriptorSet = descriptorSet;
    this->data.buffer.size = bufferSize;

    // buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* p;
    vkMapMemory(ctx.device, stagingBufferMemory, 0, bufferSize, 0, &p);
    memcpy(p, bufferData, bufferSize);
    vkUnmapMemory(ctx.device, stagingBufferMemory);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT // used as a storage buffer for compute shader
                             | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // transfer data from host to GPU
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (isVertexShaderAccessible) {
        // used as a vertex buffer for vert shader
        usage = usage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (isTransferSource) {
        usage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    createBuffer(ctx, bufferSize, usage, properties, this->data.buffer.buffer, this->data.buffer.memory);
    // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
    copyBuffer(ctx, stagingBuffer, this->data.buffer.buffer, bufferSize);

    vkDestroyBuffer(ctx.device, stagingBuffer, nullptr);
    vkFreeMemory(ctx.device, stagingBufferMemory, nullptr);

    updateDescriptorSet();
}

Resource::Resource(
    Context &ctx,
    uint32_t imageBinding,
    uint32_t samplerBinding,
    DescriptorSet *descriptorSet
) {
    this->type = Image;
    this->binding = imageBinding;
    this->data.image.samplerBinding = samplerBinding;
    this->ctx = &ctx;
    this->descriptorSet = descriptorSet;
    int textureWidth, textureHeight, textureChannels;

    // loading the image from files
    stbi_uc* pixels = stbi_load("texture.jpg", &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = textureWidth * textureHeight * STBI_rgb_alpha;
    if (!pixels) {
        throw std::runtime_error("failed to load texture!");
    }

    // making a buffer and copying the pixel data into it
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createBuffer(ctx, imageSize, usage, properties, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(ctx.device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(ctx.device, stagingBufferMemory);

    // creating a vulkan image object
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(textureWidth);
    imageInfo.extent.height = static_cast<uint32_t>(textureHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional
    if (vkCreateImage(ctx.device, &imageInfo, nullptr, &this->data.image.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    // binding the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ctx.device, this->data.image.image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(ctx, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &this->data.image.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(ctx.device, this->data.image.image, this->data.image.memory, 0);

    // preparing the image to be copied into, and then copying the pixel date from the staging buffer into it
    transitionImageLayout(ctx, 1, &this->data.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nullptr);
    copyBufferToImage(ctx, stagingBuffer, this->data.image.image, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    transitionImageLayout(ctx, 1, &this->data.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr);

    // creating the image view object
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = this->data.image.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &this->data.image.view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    // Texture Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // the higher this is the slower it is for performance
    // samplerInfo.anisotropyEnable = VK_TRUE;
    // samplerInfo.maxAnisotropy = std::min(2.0f, properties.limits.maxSamplerAnisotropy);
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(ctx.device, &samplerInfo, nullptr, &this->data.image.sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    // freeing stuff
    stbi_image_free(pixels);
    vkDestroyBuffer(ctx.device, stagingBuffer, nullptr);
    vkFreeMemory(ctx.device, stagingBufferMemory, nullptr);

    updateDescriptorSet();
}


void Resource::updateDescriptorSet() const {
    if (!descriptorSet) {
        throw std::runtime_error("DescriptorSet is null in Resource::updateDescriptorSet()");
    }

    switch (this->type) {
        case SSBO: {
            VkDescriptorSetLayoutBinding b{};
            b.binding = binding;
            b.descriptorCount = 1;
            b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            descriptorSet->addBinding(b);
            descriptorSet->addSSBOWrite(data.buffer.buffer, data.buffer.size, binding);
            break;
        }
        case Image: {
            VkDescriptorSetLayoutBinding imageBinding{};
            imageBinding.binding = binding;
            imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            imageBinding.descriptorCount = 1;
            descriptorSet->addBinding(imageBinding);
            VkDescriptorSetLayoutBinding samplerBinding{};
            samplerBinding.binding = data.image.samplerBinding;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            samplerBinding.descriptorCount = 1;
            descriptorSet->addBinding(samplerBinding);
            descriptorSet->addImageWrite(data.image.view, binding);
            descriptorSet->addSamplerWrite(data.image.view, data.image.sampler, data.image.samplerBinding);
            break;
        }
    }
}

void Resource::destroy() const {
    switch (this->type) {
        case SSBO:
            vkDestroyBuffer(ctx->device, this->data.buffer.buffer, nullptr);
            vkFreeMemory(ctx->device, this->data.buffer.memory, nullptr);
        break;
        case Image:
            vkDestroySampler(ctx->device, this->data.image.sampler, nullptr);
            vkDestroyImageView(ctx->device, this->data.image.view, nullptr);
            vkDestroyImage(ctx->device, this->data.image.image, nullptr);
            vkFreeMemory(ctx->device, this->data.image.memory, nullptr);
        break;
    }

}
