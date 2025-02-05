#ifndef MAIN_H
#define MAIN_H
#include <vector>
#include <glm/glm.hpp>

#define UXN_RAM_SIZE 65536
#define UXN_STACK_SIZE 256
#define UXN_DEV_SIZE 256

typedef struct uxn_memory {
    glm::uint pc;
    glm::uint ram[UXN_RAM_SIZE];
    glm::uint wst[UXN_STACK_SIZE];  // working stack
    glm::uint pWst;
    glm::uint rst[UXN_STACK_SIZE];  // return stack
    glm::uint pRst;
    glm::uint dev[UXN_DEV_SIZE];
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
