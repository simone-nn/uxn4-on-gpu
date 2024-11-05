cmake_minimum_required(VERSION 3.29)
project(uxn-on-gpu)

set(CMAKE_CXX_STANDARD 20)

add_executable(vulkan_test
        src/main.cpp)

# Vulkan
set(VK_ICD_FILENAMES /Users/andrei/libs/VulkanSDK/macOS/share/vulkan/icd.d/MoltenVK_icd.json)
set(VK_LAYER_PATH    /Users/andrei/libs/VulkanSDK/macOS/share/vulkan/explicit_layer.d)
set(VULKAN_DIR         /Users/andrei/libs/VulkanSDK/macOS/)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}include/)
set(VULKAN_LIBRARY     ${VULKAN_DIR}lib/)

target_include_directories(vulkan_test PRIVATE ${VULKAN_INCLUDE_DIR})
target_link_libraries(vulkan_test PRIVATE ${VULKAN_LIBRARY}libvulkan.dylib ${VULKAN_LIBRARY}libvulkan.1.3.290.dylib)

# GLFW
find_package(glfw3 3.4 REQUIRED)
target_link_libraries(vulkan_test PRIVATE glfw)

# output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
