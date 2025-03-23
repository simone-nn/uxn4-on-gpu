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
#include "IO.hpp"
#include "Console.hpp"
#include "Uxn.hpp"

#define UXN_EMULATOR_PATH "shaders/uxn_emu.spv"
#define BLIT_SHADER_PATH  "shaders/blit.spv"
#define VERT_SHADER_PATH  "shaders/shader.vert.spv"
#define FRAG_SHADER_PATH  "shaders/shader.frag.spv"

// Window Dimensions
int WIDTH = 512;
int HEIGHT = 512;

// -- Bindings --
#define SHARED_UXN_BINDING          0
#define PRIVATE_UXN_BINDING         1
#define BACKGROUND_IMAGE_BINDING    2
#define BACKGROUND_SAMPLER_BINDING  4
#define FOREGROUND_IMAGE_BINDING    3
#define FOREGROUND_SAMPLER_BINDING  5

#define VERTEX_BINDING 0
#define VERTEX_LOCATION 6
typedef struct vertex {
    glm::vec2 position;
    glm::vec2 uv;

    vertex(float x, float y, float u, float v) {
        position = glm::vec2(x, y);
        uv = glm::vec2(u, v);
    }

    vertex(glm::vec2 position, glm::vec2 uv) {
        this->position = position;
        this->uv = uv;
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = VERTEX_BINDING;
        bindingDescription.stride = sizeof(vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

        attributeDescriptions[0].binding = VERTEX_BINDING;
        attributeDescriptions[0].location = VERTEX_LOCATION;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[0].offset = 0;

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


class DeviceController {
public:
    bool enableValidationLayers;
#define H 1.0
#define T 1.0
#define L (-H)
#define Z 0.0
    std::vector<Vertex> vertices = {
        vertex(L, L, Z, Z), // first triangle
        vertex(H, L, T, Z),
        vertex(H, H, T, T),
        vertex(H, H, T, T), // second triangle
        vertex(L, H, Z, T),
        vertex(L, L, Z, Z)
    };
    const size_t VERTICES_SIZE = sizeof(Vertex) * vertices.size();
    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset" };

    DeviceController(bool enableValidationLayers, Uxn* uxn, Console* console) {
        this->enableValidationLayers = enableValidationLayers;
        this->uxn = uxn;
        uxn->debug = enableValidationLayers;
        this->console = console;
        init();
    }

    void run() {
        mainLoop();
        cleanup();
    }
private:
    Context ctx;
    Uxn *uxn;
    Console *console;
    uint32_t uxn_width, uxn_height;

    VkRenderPass renderPass;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandBuffer graphicsCommandBuffer;

    VkPipelineLayout uxnEvaluatePipelineLayout;
    VkPipeline uxnEvaluatePipeline;
    VkPipelineLayout blitPipelineLayout;
    VkPipeline blitPipeline;
    VkCommandBuffer computeCommandBuffer;

    DescriptorSetWrapper uxnDescriptorSet;
    DescriptorSetWrapper blitDescriptorSet;
    DescriptorSetWrapper graphicsDescriptorSet;
    Resource sharedUxnResource;
    Resource privateUxnResource;
    Resource backgroundImageResource;
    Resource foregroundImageResource;
    Resource vertexResource;

    VkBuffer hostDestBuffer;
    VkDeviceMemory hostDestMemory;
    VkBuffer hostSrcBuffer;
    VkDeviceMemory hostSrcMemory;
    void* hostSrcP;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence graphicsFence;
    VkFence computeInFlightFence;
    VkFence uxnEvaluationFence;
    VkFence blitFence;

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
        // Disables Retina Displays
        glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);

        // Creating the window
        ctx.window = glfwCreateWindow(WIDTH, HEIGHT, "UXN on GPU", nullptr, nullptr);

        // make the cursor invisible
        glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        // setting callbacks
        glfwSetKeyCallback(ctx.window, keyboardPressCallback);
        glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
        glfwSetCursorPosCallback(ctx.window, cursorPositionCallback);

        // initialising the keymap
        keyboardInit();
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
        std::cout << "..initSwapChain";
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

        std::cout << ": extent[w:" << ctx.swapChainExtent.width << ", h:" << ctx.swapChainExtent.height << "]\n";
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
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = ctx.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult graphicsResult = vkAllocateCommandBuffers(ctx.device, &allocInfo, &graphicsCommandBuffer);
        VkResult computeResult = vkAllocateCommandBuffers(ctx.device, &allocInfo, &computeCommandBuffer);

        if (graphicsResult != VK_SUCCESS || computeResult != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");
    }

    void initDescriptorPool() {
        std::cout << "..initDescriptorPool" << std::endl;

        // todo figure out what descriptorCount actually means, and why it needs to be set to 2
        std::array<VkDescriptorPoolSize, 4> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 2;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 2;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[2].descriptorCount = 2;
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[3].descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 3;

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
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &graphicsDescriptorSet.layout;
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

    void initComputePipeline(
        const char* shaderPath,
        VkPipeline &pipeline,
        VkPipelineLayout &pipelineLayout,
        const VkDescriptorSetLayout *descriptorLayouts,
        int descriptorCount
    ) const {
        std::cout << "..initPipeline: " << shaderPath << std::endl;

        auto compShaderCode = readFile(shaderPath);
        VkShaderModule compShaderModule = createShaderModule(compShaderCode, ctx.device);
        VkPipelineShaderStageCreateInfo compShaderStageInfo{};
        compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        compShaderStageInfo.module = compShaderModule;
        compShaderStageInfo.pName = "main";

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = descriptorCount;
        pipelineLayoutInfo.pSetLayouts = descriptorLayouts;

        if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage = compShaderStageInfo;

        if (vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        vkDestroyShaderModule(ctx.device, compShaderModule, nullptr);
    }

    void initResources() {
        std::cout << "..initResources" << std::endl;

        uxnDescriptorSet = DescriptorSetWrapper();
        blitDescriptorSet = DescriptorSetWrapper();
        graphicsDescriptorSet = DescriptorSetWrapper();

        // resource creation
        sharedUxnResource = Resource(ctx, SHARED_UXN_BINDING, &uxnDescriptorSet,
            sizeof(UxnMemory::shared), &uxn->memory->shared,
            true, false, true);
        privateUxnResource = Resource(ctx, PRIVATE_UXN_BINDING, &uxnDescriptorSet,
            sizeof(UxnMemory::_private), &uxn->memory->_private,
            true, false, false);

        backgroundImageResource = Resource(ctx, BACKGROUND_IMAGE_BINDING, BACKGROUND_SAMPLER_BINDING,
            &blitDescriptorSet, &graphicsDescriptorSet, {uxn_width, uxn_height, 0});
        foregroundImageResource = Resource(ctx, FOREGROUND_IMAGE_BINDING, FOREGROUND_SAMPLER_BINDING,
            &blitDescriptorSet, &graphicsDescriptorSet, {uxn_width, uxn_height, 0});
        vertexResource = Resource(ctx, VERTEX_LOCATION, &graphicsDescriptorSet,
            VERTICES_SIZE, vertices.data(),
            false, true, false);

        uxnDescriptorSet.initialise(ctx);
        blitDescriptorSet.initialise(ctx);
        graphicsDescriptorSet.initialise(ctx);

        // host staging buffer
        createBuffer(ctx, sizeof(UxnMemory::shared),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            hostDestBuffer, hostDestMemory);
        createBuffer(ctx, sizeof(UxnMemory::shared),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            hostSrcBuffer, hostSrcMemory);
        if (vkMapMemory(ctx.device, hostSrcMemory, 0, sizeof(UxnMemory::shared), 0, &hostSrcP) != VK_SUCCESS) {
            std::cerr << "Failed to map memory!" << std::endl;
        }
    }

    void initSync() {
        std::cout << "..initSync" << std::endl;
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(ctx.device, &fenceInfo, nullptr, &uxnEvaluationFence) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &blitFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects!");
        }

        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &graphicsFence) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceInfo, nullptr, &computeInFlightFence) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects!");
        }
    }

    void updateUxnConstants() {
        std::cout << "..updateUxnConstants" << std::endl;

        uxn_width  = static_cast<uint32_t>(WIDTH * H);
        uxn_height = static_cast<uint32_t>(HEIGHT * H);

        // Screen Device
        to_uxn_mem2(static_cast<uint16_t>(uxn_width), &uxn->memory->shared.dev[0x22]);
        to_uxn_mem2(static_cast<uint16_t>(uxn_height), &uxn->memory->shared.dev[0x24]);

        //todo populate all uxn constants for all devices
        // uxn.dev[0x17] = argc > 2;
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
        updateUxnConstants();
        initResources();
        std::array blitLayouts = {uxnDescriptorSet.layout, blitDescriptorSet.layout};
        initComputePipeline(UXN_EMULATOR_PATH, uxnEvaluatePipeline, uxnEvaluatePipelineLayout, &uxnDescriptorSet.layout, 1);
        initComputePipeline(BLIT_SHADER_PATH, blitPipeline, blitPipelineLayout, blitLayouts.data(), blitLayouts.size());
        initFrameBuffers();
        initGraphicsPipeline();
        initSync();
    }

    void copyDeviceMemToHost(UxnMemory* target) {
        // copy from ssbo buffer to host staging buffer
        auto size = sharedUxnResource.data.buffer.size;
        copyBuffer(ctx, sharedUxnResource.data.buffer._, hostDestBuffer, size);

        void* data;
        if (vkMapMemory(ctx.device, hostDestMemory, 0, size, 0, &data) != VK_SUCCESS) {
            std::cerr << "Failed to map memory!" << std::endl;
            return;
        }

        auto* mappedMemory = static_cast<UxnMemory*>(data);
        memset(&target->shared, 0, size);
        memcpy(&target->shared, mappedMemory, size);
        vkUnmapMemory(ctx.device, hostDestMemory);
    }

    void copyHostMemToDevice(const UxnMemory* source) {
        // copy data to staging buffer
        memcpy(hostSrcP, &source->shared, sizeof(UxnMemory::shared));

        // copy from host staging buffer to ssbo buffer
        copyBuffer(ctx, hostSrcBuffer, sharedUxnResource.data.buffer._, sizeof(UxnMemory::shared));
    }

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
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout,
            0,1, &graphicsDescriptorSet.set, 0, nullptr);

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
            vkCmdBindVertexBuffers(cmdBuffer, VERTEX_BINDING, 1, &vertexResource.data.buffer._, offsets);

            vkCmdDraw(cmdBuffer, vertices.size(), 1, 0, 0);
        }
        vkCmdEndRenderPass(cmdBuffer);

        if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void clearImage(VkCommandBuffer cmdBuffer) {
        bool singleTimeBuffer = cmdBuffer == VK_NULL_HANDLE;
        if (singleTimeBuffer)
            cmdBuffer = beginSingleTimeCommands(ctx);

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VkClearColorValue foregroundColor = {};
        foregroundColor.float32[0] = 0.0f;
        foregroundColor.float32[1] = 0.0f;
        foregroundColor.float32[2] = 0.0f;
        foregroundColor.float32[3] = 0.0f;

        auto back = uxn->getBackgroundColor();
        VkClearColorValue backgroundColor = {};
        backgroundColor.float32[0] = back.x;
        backgroundColor.float32[1] = back.y;
        backgroundColor.float32[2] = back.z;
        backgroundColor.float32[3] = back.w;

        vkCmdClearColorImage(cmdBuffer, backgroundImageResource.data.image._, VK_IMAGE_LAYOUT_GENERAL, &backgroundColor, 1, &subresourceRange);
        vkCmdClearColorImage(cmdBuffer, foregroundImageResource.data.image._, VK_IMAGE_LAYOUT_GENERAL, &foregroundColor, 1, &subresourceRange);

        if (singleTimeBuffer)
             endSingleTimeCommands(ctx, cmdBuffer);
    }

    /// Transition the images formats to edit mode for the blit shader
    void transitionImagesToEditLayout(VkCommandBuffer cmdBuffer) {
        std::array images = {backgroundImageResource.data.image._, foregroundImageResource.data.image._};
        transitionImageLayout(ctx, 2, images.data(),
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_GENERAL,
                              cmdBuffer);
    }

    /// Transition the images formats to read mode for graphics
    void transitionImagesToReadLayout(VkCommandBuffer cmdBuffer) {
        std::array images = {backgroundImageResource.data.image._, foregroundImageResource.data.image._};
        transitionImageLayout(ctx, 2, images.data(),
                              VK_IMAGE_LAYOUT_GENERAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              cmdBuffer);
    }

    void uxnEvalShader(bool clear_image) {
        // --- UXN evaluation submission ---
        vkQueueWaitIdle(ctx.computeQueue);
        vkResetFences(ctx.device, 1, &uxnEvaluationFence);
        vkResetCommandBuffer(computeCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, uxnEvaluatePipeline);
        vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, uxnEvaluatePipelineLayout,
            0,1, &uxnDescriptorSet.set, 0, nullptr);

        if (clear_image)
            clearImage(computeCommandBuffer);

        vkCmdDispatch(computeCommandBuffer, 1, 1, 1);

        if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCommandBuffer;
        if (vkQueueSubmit(ctx.computeQueue, 1, &submitInfo, uxnEvaluationFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        }
        // wait for uxn step to be done
        vkWaitForFences(ctx.device, 1, &uxnEvaluationFence, VK_TRUE, UINT64_MAX);
    }

    void blitShader() {
        // --- Blit submission ---
        vkResetFences(ctx.device, 1, &blitFence);
        vkResetCommandBuffer(computeCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        std::array descriptors = {uxnDescriptorSet.set, blitDescriptorSet.set};
        vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blitPipeline);
        vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blitPipelineLayout,
            0,descriptors.size(), descriptors.data(), 0, nullptr);

        vkCmdDispatch(computeCommandBuffer, 1, 1, 1);

        if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCommandBuffer;
        if (vkQueueSubmit(ctx.computeQueue, 1, &submitInfo, blitFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        }

        // wait for blit step to be done
        vkWaitForFences(ctx.device, 1, &blitFence, VK_TRUE, UINT64_MAX);
    }

    void graphicsStep() {
        // Graphics submission
        // wait for previous frame to finish
        vkWaitForFences(ctx.device, 1, &graphicsFence, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx.device, 1, &graphicsFence);

        // get the next image:
        uint32_t imageIndex;
        vkAcquireNextImageKHR(ctx.device, ctx.swapChain, UINT64_MAX, imageAvailableSemaphore,
                              VK_NULL_HANDLE, &imageIndex);

        // record commands in the current command buffer:
        vkResetCommandBuffer(graphicsCommandBuffer, 0);
        recordGraphicsCommandBuffer(graphicsCommandBuffer, imageIndex);

        // submit info that accompanies the commands:
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::array waitSemaphores = {imageAvailableSemaphore};
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &graphicsCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        // Graphic Commands get submitted:
        if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, graphicsFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // present commands:
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        VkSwapchainKHR swapChains[] = {ctx.swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        // Present Commands get submitted:
        vkQueuePresentKHR(ctx.presentQueue, &presentInfo);
    }

    bool doCallback(uxn_device device, bool did_graphics) {
        switch (device) {
            case uxn_device::Console:
                return console->notEmpty();
            case uxn_device::Screen:
                return !did_graphics;
            case uxn_device::Mouse:
                return mouse.used;
            case uxn_device::Controller:
                return keyboard.used;
            default:
               return false;
        }
    }

    void mainLoop() {
        constexpr int target_FPS = 60;
        constexpr std::chrono::milliseconds frame_duration(1000 / target_FPS);

        glm::uint halt_code = 0;
        bool in_vector = true, did_graphics = true;
        bool cleared = false;
        auto last_frame_time = std::chrono::steady_clock::now();
        auto current_vector = uxn_device::Null;
        int callback_index = 0;

        while (!glfwWindowShouldClose(ctx.window) && !uxn->programTerminated()) {
            glfwPollEvents();

            if (!in_vector) {
                // pick a new vector to execute
                auto callback = CALLBACK_DEVICES[callback_index];
                if (uxn->deviceCallbackVectors.contains(callback) && doCallback(callback, did_graphics)) {
                    uxn->prepareCallback(callback);
                    copyHostMemToDevice(uxn->memory);
                    in_vector = true;
                    current_vector = callback;
                }
                callback_index = (callback_index + 1) % static_cast<int>(CALLBACK_DEVICES.size());
            }

            if (in_vector) {
                // compute steps
                uxnEvalShader(false);
                // when it halts from a uxn eval, we need to figure out whenever or not
                // to clear the screen before the blit shader
                // the issue is: we need to clear the screen before we draw to it,
                // and we should only draw onto the screen if it has been drawn
                if (!cleared) {
                    copyDeviceMemToHost(uxn->memory);
                    if (uxn->maskFlag(DRAW_PIXEL_FLAG) || uxn->maskFlag(DRAW_SPRITE_FLAG)) {
                        cleared = true;
                        clearImage(nullptr);
                    }
                }
                blitShader();
                copyDeviceMemToHost(uxn->memory);
                uxn->handleUxnIO();

                // decide if the vector is finished
                halt_code = uxn->memory->shared.halt;

                if (halt_code == 1) {
                    in_vector = false;
                    if (current_vector == uxn_device::Screen) { did_graphics = true; }

                    // std::cout << "Vector finished: " << static_cast<int>(current_vector)
                    //     << ", pc: 0x" << std::hex << uxn->memory->shared.pc
                    //     << " clear_frame: " << clear_frame
                    //     << " clear_next_frame: " << clear_next_frame << "\n"
                    //     << " last instructions: " << uxn->memory->_private.ram[uxn->memory->shared.pc-1]
                    //     << ", " << uxn->memory->_private.ram[uxn->memory->shared.pc-2]
                    //     << ", " << uxn->memory->_private.ram[uxn->memory->shared.pc-3]
                    //     << std::dec << std::endl;
                }
            }

            // graphics step: only enter if it is time to draw a frame again (60 FPS)
            auto now_time = std::chrono::steady_clock::now();
            auto elapsed_since_frame = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_frame_time);
            if ((elapsed_since_frame >= frame_duration) && halt_code == 1 && did_graphics) {
                transitionImagesToReadLayout(nullptr);
                graphicsStep();
                transitionImagesToEditLayout(nullptr);

                last_frame_time = std::chrono::steady_clock::now();
                callback_index = 0;
                did_graphics = false;
                cleared = false;
            }
            if (!in_vector)
                current_vector = uxn_device::Null;

            // check if crashed
            if (halt_code == 4) {
                copyHostMemToDevice(uxn->memory);
                std::cerr << "Estimated last opcode before crash: 0x" << std::hex
                          << uxn->memory->_private.ram[uxn->memory->shared.pc-1]
                          << std::dec << "\n";
                throw std::runtime_error("VM encountered unknown opcode!");
            }
        }
        if (uxn->programTerminated()) {
            std::cout << "Uxn Program Terminated with exit code: 0x" << std::hex
            << static_cast<int>(from_uxn_mem(&uxn->memory->shared.dev[0x0f])) - 0x80 << std::dec;
        }
    }

    void cleanup() {
        vkDeviceWaitIdle(ctx.device);
        delete uxn;
        console->stop();
        uxnDescriptorSet.destroy(ctx);
        blitDescriptorSet.destroy(ctx);
        graphicsDescriptorSet.destroy(ctx);
        vkUnmapMemory(ctx.device, hostSrcMemory);
        vkDestroyBuffer(ctx.device, hostDestBuffer, nullptr);
        vkFreeMemory(ctx.device, hostDestMemory, nullptr);
        vkDestroyBuffer(ctx.device, hostSrcBuffer, nullptr);
        vkFreeMemory(ctx.device, hostSrcMemory, nullptr);
        vkDestroySemaphore(ctx.device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(ctx.device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(ctx.device, graphicsFence, nullptr);
        vkDestroyFence(ctx.device, computeInFlightFence, nullptr);
        vkDestroyFence(ctx.device, uxnEvaluationFence, nullptr);
        vkDestroyFence(ctx.device, blitFence, nullptr);
        sharedUxnResource.destroy();
        privateUxnResource.destroy();
        backgroundImageResource.destroy();
        foregroundImageResource.destroy();
        vertexResource.destroy();
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        for (auto framebuffer : ctx.swapChainFramebuffers) {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
        vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
        vkDestroyPipeline(ctx.device, uxnEvaluatePipeline, nullptr);
        vkDestroyPipeline(ctx.device, blitPipeline, nullptr);
        vkDestroyPipelineLayout(ctx.device, graphicsPipelineLayout, nullptr);
        vkDestroyPipelineLayout(ctx.device, uxnEvaluatePipelineLayout, nullptr);
        vkDestroyPipelineLayout(ctx.device, blitPipelineLayout, nullptr);
        vkDestroyRenderPass(ctx.device, renderPass, nullptr);
        for (auto imageView : ctx.swapChainImageViews) {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
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
    auto console = new Console;
    auto uxn = new Uxn(filename, console);

    DeviceController app(true, uxn, console);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
