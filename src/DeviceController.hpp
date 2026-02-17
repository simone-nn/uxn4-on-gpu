#ifndef MAIN_H
#define MAIN_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

#define LOG(s) if(debug) std::cout << s << std::endl

typedef struct context {
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDevice device; // logical device
    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
} Context;

std::vector<char> readFile(const std::string& filename);

VkCommandBuffer beginSingleTimeCommands(const Context &ctx);

void endSingleTimeCommands(const Context &ctx, VkCommandBuffer commandBuffer);

#endif //MAIN_H
