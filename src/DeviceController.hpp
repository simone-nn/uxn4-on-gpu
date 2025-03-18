#ifndef MAIN_H
#define MAIN_H
#include <vector>

// mouse stuff
inline struct MouseState {
    bool used = false;
    uint16_t cursor_x, cursor_y;
    bool mouse1, mouse2, mouse3;
} mouse;

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
