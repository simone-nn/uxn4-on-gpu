#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <thread>
#include <glm/glm.hpp>
#include "Resource.hpp"
#include "DeviceController.hpp"
#include "Uxn.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VERT_SHADER_PATH  "shaders/vert.spv"
#define FRAG_SHADER_PATH  "shaders/frag.spv"
#define COMP_SHADER_PATH  "shaders/comp.spv"
#define UXN_EMULATOR_PATH "shaders/uxn_emu.spv"

typedef struct vertex {
    glm::vec2 position;
    glm::vec2 padding;

    vertex(float x, float y) {
        position = glm::vec2(x, y);
        padding = glm::vec2(0, 0);
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(vertex, position);

        return attributeDescriptions;
    }
} Vertex;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queueFamilyCount, queueFamilies.data());

    for (int i = 0; i < queueFamilyCount; ++i) {
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.graphicsAndComputeFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
                device, indices.graphicsAndComputeFamily.value(), surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) break;
    }

    return indices;
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
                                         &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
                                             &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface,
                &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

std::vector<char> readFile(const std::string& filename) {
    // std::ios::ate means start reading at the end of the file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open file: '" + filename + "'!");

    // reading the file
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<long>(fileSize));
    file.close();

    return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto&[extensionName, _] : availableExtensions) {
        requiredExtensions.erase(extensionName);
    }

    return requiredExtensions.empty();
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<const char*> deviceExtensions) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device, std::move(deviceExtensions));

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate
           && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkCommandBuffer beginSingleTimeCommands(const Context &ctx) {
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

    return commandBuffer;

}

void endSingleTimeCommands(const Context &ctx, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);

    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

void transitionImageLayout(const Context &ctx, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ctx);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
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
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );


    endSingleTimeCommands(ctx, commandBuffer);
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


class DeviceController {
public:
    int WIDTH = 800;
    int HEIGHT = 600;
    bool enableValidationLayers;
    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset" };

    DeviceController(bool enableValidationLayers, Uxn* uxn) {
        this->enableValidationLayers = enableValidationLayers;
        this->uxn = uxn;
        init();
    }

    void run() {
        std::cout << "Hello world!\n";
        mainLoop();
        cleanup();
    }
private:
    Context ctx;
    Uxn *uxn;

    VkRenderPass renderPass;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;

    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    std::vector<VkCommandBuffer> computeCommandBuffers;

    Resource uxnResource;
    VkBuffer hostStagingBuffer;
    VkDeviceMemory hostStagingMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> computeInFlightFences;
    std::vector<VkFence> uxnEvaluationFences;

    const int MAX_FRAME_STEPS = 2;
    uint32_t frameStep = 0;

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) return false;
        }
        return true;
    }

    void initWindow() {
        std::cout << "..initWindow" << std::endl;
        glfwInit();
        // Tell GLFW not to use OpenGL.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Tell GLFW that the window shouldn't be resizable.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        // Creating the window
        ctx.window = glfwCreateWindow(WIDTH, HEIGHT, "UXN on GPU", nullptr, nullptr);
    }

    void initVkInstance() {
        std::cout << "..initVkInstance" << std::endl;
        /// Extensions
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        extensions.reserve(glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            extensions.emplace_back(glfwExtensions[i]);
        }
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); // required on macOSX

        /// Instance
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "UXN on GPU";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // required on macOS
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        /// Validation Layers
        if (enableValidationLayers) {
            if (!checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // creating the instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &ctx.instance);
        if (result != VK_SUCCESS) {
            if (result == VK_ERROR_LAYER_NOT_PRESENT) {
                throw std::runtime_error("failed to create instance: VK_ERROR_LAYER_NOT_PRESENT\n");
            }
            if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
                throw std::runtime_error("failed to create instance: VK_ERROR_EXTENSION_NOT_PRESENT\n");
            }
            if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
                throw std::runtime_error("failed to create instance: VK_ERROR_INCOMPATIBLE_DRIVER\n");
            }
            std::cout << "VkResult:" << result;
            throw std::runtime_error("failed to create instance: unknown error\n");
        }
    }

    void initSurface() {
        std::cout << "..initSurface" << std::endl;
        if (glfwCreateWindowSurface(ctx.instance, ctx.window, nullptr, &ctx.surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }

    void initPhysicalDevice() {
        std::cout << "..initPhysicalDevice: ";
        uint32_t deviceCount = 0;
        ctx.physicalDevice = VK_NULL_HANDLE;
        vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());

        std::cout << devices.size() << " devices found.\n";
        for (const auto &p_device: devices) {
            if (isDeviceSuitable(p_device, ctx.surface, deviceExtensions)) {
                ctx.physicalDevice = p_device;
                break;
            }
        }

        if (ctx.physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("failed to find a suitable GPU!");
    }

    void initLogicalDevice() {
        std::cout << "..initLogicalDevice" << std::endl;
        auto [graphicsAndComputeFamily, presentFamily] = findQueueFamilies(ctx.physicalDevice, ctx.surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {graphicsAndComputeFamily.value(), presentFamily.value()};
        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Specify used device features
        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pNext = nullptr;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(ctx.physicalDevice, &createInfo, nullptr, &ctx.device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(ctx.device, presentFamily.value(), 0, &ctx.presentQueue);
        vkGetDeviceQueue(ctx.device, graphicsAndComputeFamily.value(), 0, &ctx.graphicsQueue);
        vkGetDeviceQueue(ctx.device, graphicsAndComputeFamily.value(), 0, &ctx.computeQueue);
    }

    void initDebug() {
        if (enableValidationLayers) {
            std::cout << "..initDebug" << std::endl;
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
            createInfo.pUserData = nullptr; // Optional

            if (CreateDebugUtilsMessengerEXT(ctx.instance, &createInfo,nullptr, &ctx.debugMessenger) != VK_SUCCESS)
                throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void initSwapChain() {
        std::cout << "..initSwapChain" << std::endl;
        auto [capabilities, formats, presentModes] = querySwapChainSupport(ctx.physicalDevice, ctx.surface);

        auto [surfaceFormat, surfaceColorSpace] = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D extent = chooseSwapExtent(capabilities, ctx.window);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = ctx.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat;
        createInfo.imageColorSpace = surfaceColorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        auto [graphicsAndComputeFamily, presentFamily] = findQueueFamilies(ctx.physicalDevice, ctx.surface);
        uint32_t queueFamilyIndices[] = {graphicsAndComputeFamily.value(), presentFamily.value()};

        if (graphicsAndComputeFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        if (vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &imageCount, nullptr);
        ctx.swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &imageCount, ctx.swapChainImages.data());
        ctx.swapChainImageFormat = surfaceFormat;
        ctx.swapChainExtent = extent;
    }

    void initImageViews() {
        std::cout << "..initImageViews" << std::endl;
        ctx.swapChainImageViews.resize(ctx.swapChainImages.size());
        for (size_t i = 0; i < ctx.swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = ctx.swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = ctx.swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(ctx.device, &createInfo, nullptr, &ctx.swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void initRenderPass() {
        std::cout << "..initRenderPass" << std::endl;
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = ctx.swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // this can be a compute subpass too!
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void initFrameBuffers() {
        std::cout << "..initFrameBuffers" << std::endl;
        ctx.swapChainFramebuffers.resize(ctx.swapChainImageViews.size());
        for (size_t i = 0; i < ctx.swapChainImageViews.size(); i++) {
            VkImageView attachments[] = { ctx.swapChainImageViews[i] };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = ctx.swapChainExtent.width;
            framebufferInfo.height = ctx.swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void initCommands() {
        std::cout << "..initCommands" << std::endl;
        // Command Pool
        auto [graphicsAndComputeFamily, presentFamily] = findQueueFamilies(ctx.physicalDevice, ctx.surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsAndComputeFamily.value();

        if (vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }

        // Command Buffers
        graphicsCommandBuffers.resize(MAX_FRAME_STEPS);
        computeCommandBuffers.resize(MAX_FRAME_STEPS);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = ctx.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAME_STEPS);

        VkResult graphicsResult = vkAllocateCommandBuffers(ctx.device, &allocInfo, graphicsCommandBuffers.data());
        VkResult computeResult = vkAllocateCommandBuffers(ctx.device, &allocInfo, computeCommandBuffers.data());

        if (graphicsResult != VK_SUCCESS || computeResult != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");
    }

    void initTexture() {
        std::cout << "..initTexture" << std::endl;
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
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        if (vkCreateImage(ctx.device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // binding the image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(ctx.device, textureImage, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(ctx, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(ctx.device, textureImage, textureImageMemory, 0);

        // preparing the image to be copied into, and then copying the pixel date from the staging buffer into it
        transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(ctx, stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
        transitionImageLayout(ctx, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // freeing stuff
        stbi_image_free(pixels);
        vkDestroyBuffer(ctx.device, stagingBuffer, nullptr);
        vkFreeMemory(ctx.device, stagingBufferMemory, nullptr);
    }

    void initVertexBuffer() {
        std::cout << "..initVertexBuffer" << std::endl;
        //todo
    }

    void initDescriptorPool() {
        std::cout << "..initDescriptorPool" << std::endl;
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAME_STEPS);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAME_STEPS);

        if (vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &ctx.descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void initGraphicsPipeline() {
        std::cout << "..initGraphicsPipeline" << std::endl;
        // "../" needs to be added in front of the paths because CLion puts the executable in cmake-build-debug
        // will have to be different in a production build
        // TODO: hard coded path for Debug compilation
        auto vertShaderCode = readFile(VERT_SHADER_PATH);
        auto fragShaderCode = readFile(FRAG_SHADER_PATH);

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, ctx.device);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, ctx.device);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDesc = Vertex::getBindingDescription();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        auto attributeDesc = Vertex::getAttributeDescriptions();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // TODO figure out how many of these things are actually optional, and trim the rest
        // ? is this necessary
        std::vector dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(ctx.swapChainExtent.width);
        viewport.height = static_cast<float>(ctx.swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = ctx.swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1,
                                      &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        // Shader Modules were copied into the pipeline, so they can be destroyed
        vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);
        vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
    }

    void initComputePipeline() {
        std::cout << "..initComputePipeline" << std::endl;
        auto compShaderCode = readFile(UXN_EMULATOR_PATH);
        VkShaderModule compShaderModule = createShaderModule(compShaderCode, ctx.device);
        VkPipelineShaderStageCreateInfo compShaderStageInfo{};
        compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        compShaderStageInfo.module = compShaderModule;
        compShaderStageInfo.pName = "main";

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &ctx.descriptorSetLayout;

        if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = computePipelineLayout;
        pipelineInfo.stage = compShaderStageInfo;

        if (vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        vkDestroyShaderModule(ctx.device, compShaderModule, nullptr);
    }

    void initResources() {
        std::cout << "..initResources" << std::endl;

        // descriptor set layout creation
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = 0;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &layoutBinding;

        if (vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &ctx.descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }

        // resource creation
        uxnResource = Resource(ctx, 0, sizeof(UxnMemory), uxn->memory,
            false, false, true);
        uxnResource.updateDescriptorSets(uxnResource.buffer, sizeof(UxnMemory));

        // host staging buffer
        createBuffer(ctx, sizeof(UxnMemory),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            hostStagingBuffer, hostStagingMemory);
    }

    void initSync() {
        std::cout << "..initSync" << std::endl;
        imageAvailableSemaphores.resize(MAX_FRAME_STEPS);
        renderFinishedSemaphores.resize(MAX_FRAME_STEPS);
        computeFinishedSemaphores.resize(MAX_FRAME_STEPS);
        inFlightFences.resize(MAX_FRAME_STEPS);
        computeInFlightFences.resize(MAX_FRAME_STEPS);
        uxnEvaluationFences.resize(MAX_FRAME_STEPS);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAME_STEPS; i++) {
            if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(ctx.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS ||
                vkCreateFence(ctx.device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS ||
                vkCreateFence(ctx.device, &fenceInfo, nullptr, &uxnEvaluationFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("failed to create synchronization objects!");
                }
        }
    }

    void init() {
        std::cout << "Initialising the Device Controller:" << std::endl;
        initWindow();
        initVkInstance();
        initSurface();
        initPhysicalDevice();
        initLogicalDevice();
        initDebug();
        initCommands();
        initSwapChain();
        initImageViews();
        initRenderPass();
        initDescriptorPool();
        initResources();
        initTexture();
        initVertexBuffer();
        initComputePipeline();
        initFrameBuffers();
        initGraphicsPipeline();
        initSync();
    }

    // todo update recordGraphicsCommandBuffer
    void recordGraphicsCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = ctx.swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = ctx.swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        { // Render Pass
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(ctx.swapChainExtent.width);
            viewport.height = static_cast<float>(ctx.swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = ctx.swapChainExtent;
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            VkDeviceSize offsets[] = {0};
            // vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &verticesStorage[frameStep].buffer, offsets);

            // vkCmdDraw(cmdBuffer, VERTEX_COUNT, 1, 0, 0);
        }
        vkCmdEndRenderPass(cmdBuffer);

        if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void uxnStep() {
        // Compute submission
        vkResetFences(ctx.device, 1, &uxnEvaluationFences[frameStep]);
        vkResetCommandBuffer(computeCommandBuffers[frameStep], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(computeCommandBuffers[frameStep], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdBindPipeline(computeCommandBuffers[frameStep], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(computeCommandBuffers[frameStep], VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout,
            0,1, &uxnResource.descriptorSet, 0, nullptr);

        // TODO does the memory barrier do anything?
        VkMemoryBarrier memoryBarrier{};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(computeCommandBuffers[frameStep], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

        vkCmdDispatch(computeCommandBuffers[frameStep], 1, 1, 1);

        if (vkEndCommandBuffer(computeCommandBuffers[frameStep]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCommandBuffers[frameStep];
        // submitInfo.signalSemaphoreCount = 1;
        // submitInfo.pSignalSemaphores = &computeFinishedSemaphores[frameStep];
        if (vkQueueSubmit(ctx.computeQueue, 1, &submitInfo, uxnEvaluationFences[frameStep]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        }
        // wait for uxn step to be done
        vkWaitForFences(ctx.device, 1, &uxnEvaluationFences[frameStep], VK_TRUE, UINT64_MAX);
    }

    void drawFrame() {
        // Graphics submission
        // Wait for previous frame to finish drawing
        // vkWaitForFences(ctx.device, 1, &inFlightFences[frameStep], VK_TRUE, UINT64_MAX);
        // vkResetFences(ctx.device, 1, &inFlightFences[frameStep]);

        // get the next image:
        // uint32_t imageIndex;
        // vkAcquireNextImageKHR(ctx.device, ctx.swapChain, UINT64_MAX, imageAvailableSemaphores[frameStep],
        //                       VK_NULL_HANDLE, &imageIndex);

        // record commands in the current command buffer:
        // vkResetCommandBuffer(graphicsCommandBuffers[frameStep], 0);
        // recordGraphicsCommandBuffer(graphicsCommandBuffers[frameStep], imageIndex);

        // submit info that accompanies the commands:
        // submitInfo = {};
        // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // VkSemaphore waitSemaphores[] = {computeFinishedSemaphores[frameStep], imageAvailableSemaphores[frameStep]};
        // submitInfo.waitSemaphoreCount = 2;
        // submitInfo.pWaitSemaphores = waitSemaphores;
        // VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        // submitInfo.pWaitDstStageMask = waitStages;
        // submitInfo.commandBufferCount = 1;
        // submitInfo.pCommandBuffers = &graphicsCommandBuffers[frameStep];
        // submitInfo.signalSemaphoreCount = 1;
        // submitInfo.pSignalSemaphores = &renderFinishedSemaphores[frameStep];
        //
        // // Graphic Commands get submitted:
        // if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, inFlightFences[frameStep]) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to submit draw command buffer!");
        // }

        // present commands:
        // VkPresentInfoKHR presentInfo{};
        // presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // presentInfo.waitSemaphoreCount = 1;
        // presentInfo.pWaitSemaphores = &renderFinishedSemaphores[frameStep];
        // VkSwapchainKHR swapChains[] = {ctx.swapChain};
        // presentInfo.swapchainCount = 1;
        // presentInfo.pSwapchains = swapChains;
        // presentInfo.pImageIndices = &imageIndex;
        // presentInfo.pResults = nullptr; // Optional
        //
        // // Present Commands get submitted:
        // vkQueuePresentKHR(ctx.presentQueue, &presentInfo);
    }

    void copyDeviceUxnMemory(UxnMemory* target, uint32_t frameIndex) {
        // copy from ssbo buffer to host staging buffer
        copyBuffer(ctx, uxnResource.buffer, hostStagingBuffer, sizeof(UxnMemory));

        void* data;
        if (vkMapMemory(ctx.device, hostStagingMemory, 0, sizeof(UxnMemory), 0, &data) != VK_SUCCESS) {
            std::cerr << "Failed to map memory!" << std::endl;
            return;
        }

        auto* mappedMemory = static_cast<UxnMemory*>(data);
        memset(target, 0, sizeof(UxnMemory));
        memcpy(target, mappedMemory, sizeof(UxnMemory));
        vkUnmapMemory(ctx.device, hostStagingMemory);
    }

    void mainLoop() {
        constexpr int TOTAL_STEPS = 10;

        int step = 0;
        std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
        while (!glfwWindowShouldClose(ctx.window) && step < TOTAL_STEPS) {

            // Main Loop:
            glfwPollEvents();
            uxnStep();
            drawFrame();

            // Handle IO
            copyDeviceUxnMemory(uxn->memory, frameStep);
            uxn->handleUxnIO();
            uxn->outputToFile("output.txt");

            // Print elapsed time:
            std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now_time - last_time).count();
            last_time = now_time;
            std::cout << "Frame " << step << ", time: " << static_cast<double>(elapsed)/1000000.0 << "[s]\n";

            // Iterate frame counters:
            frameStep = (frameStep + 1) % MAX_FRAME_STEPS;
            step++;
        }
    }

    void cleanup() {
        delete uxn;
        vkDestroyImage(ctx.device, textureImage, nullptr);
        vkFreeMemory(ctx.device, textureImageMemory, nullptr);
        vkDestroyBuffer(ctx.device, hostStagingBuffer, nullptr);
        vkFreeMemory(ctx.device, hostStagingMemory, nullptr);
        for (size_t i = 0; i < MAX_FRAME_STEPS; i++) {
            vkDestroySemaphore(ctx.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(ctx.device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(ctx.device, computeFinishedSemaphores[i], nullptr);
            vkDestroyFence(ctx.device, inFlightFences[i], nullptr);
            vkDestroyFence(ctx.device, computeInFlightFences[i], nullptr);
            vkDestroyFence(ctx.device, uxnEvaluationFences[i], nullptr);
        }
        uxnResource.destroy();
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        for (auto framebuffer : ctx.swapChainFramebuffers) {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
        vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
        vkDestroyPipeline(ctx.device, computePipeline, nullptr);
        vkDestroyPipelineLayout(ctx.device, graphicsPipelineLayout, nullptr);
        vkDestroyPipelineLayout(ctx.device, computePipelineLayout, nullptr);
        vkDestroyRenderPass(ctx.device, renderPass, nullptr);
        for (auto imageView : ctx.swapChainImageViews) {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
        vkDestroyDescriptorSetLayout(ctx.device, ctx.descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(ctx.device, ctx.descriptorPool, nullptr);
        vkDestroySwapchainKHR(ctx.device, ctx.swapChain, nullptr); // before device
        vkDestroyDevice(ctx.device, nullptr);
        // graphics queue is implicitly destroyed with logical device
        if (enableValidationLayers)
            DestroyDebugUtilsMessengerEXT(ctx.instance, ctx.debugMessenger, nullptr);
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        vkDestroyInstance(ctx.instance, nullptr);
        // physicalDevice is implicitly destroyed when instance is
        glfwDestroyWindow(ctx.window);
        glfwTerminate();
    }
};

int main(int nargs, char** args) {
    // Check if filename is provided
    if (nargs < 2) {
        std::cerr << "Usage: " << args[0] << " <filename>\n";
        return EXIT_FAILURE;
    }
    const char* filename = args[1];
    auto uxn = new Uxn(filename);

    DeviceController app(true, uxn);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
