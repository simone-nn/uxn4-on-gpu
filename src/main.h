#ifndef MAIN_H
#define MAIN_H
#include <vector>

#define UXN_RAM_SIZE 65536
#define UXN_STACK_SIZE 256
#define UXN_DEV_SIZE 256

typedef struct uxn_stack {
    uint8_t dat[UXN_STACK_SIZE];
    uint8_t ptr;
} UxnStack;

typedef struct uxn_memory {
    uint8_t ram[UXN_RAM_SIZE];
    UxnStack wst;  // working stack
    UxnStack rst;  // return stack
    uint8_t dev[UXN_DEV_SIZE];
} UxnMemory;

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
    VkDescriptorSetLayout descriptorSetLayout;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
} Context;

#endif //MAIN_H
