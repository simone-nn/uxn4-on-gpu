cmake_minimum_required(VERSION 3.29)
project(uxn-on-gpu)

set(CMAKE_CXX_STANDARD 20)

#message(STATUS ${CMAKE_BINARY_DIR},${CMAKE_SOURCE_DIR})

# Shader compilation
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/uxn_emu.spv
        COMMAND ${CMAKE_SOURCE_DIR}/compile_shaders.sh
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/uxn_emu.comp
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/blit.comp
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.vert.glsl
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.frag.glsl
        COMMENT "Compiling shaders..."
)
# Define a target for shader compilation
add_custom_target(compile_shaders ALL
        DEPENDS ${CMAKE_BINARY_DIR}/shaders/uxn_emu.spv
)

# Copy resources folder (No longer used)
# configure_file(${CMAKE_SOURCE_DIR}/resources/default_background.png ${CMAKE_BINARY_DIR}/resources/default_background.png COPYONLY)
# configure_file(${CMAKE_SOURCE_DIR}/resources/default_foreground.png ${CMAKE_BINARY_DIR}/resources/default_foreground.png COPYONLY)

add_executable(uxn-on-gpu
        ${CMAKE_SOURCE_DIR}/src/Resource.hpp
        ${CMAKE_SOURCE_DIR}/src/DeviceController.hpp
        ${CMAKE_SOURCE_DIR}/src/Uxn.hpp
        ${CMAKE_SOURCE_DIR}/src/DeviceController.cpp
        ${CMAKE_SOURCE_DIR}/src/Resource.cpp
        ${CMAKE_SOURCE_DIR}/src/Uxn.cpp)

add_dependencies(uxn-on-gpu compile_shaders)

# CLion fix
# set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk)

# Vulkan
set(VK_ICD_FILENAMES /Users/andrei/libs/VulkanSDK/macOS/share/vulkan/icd.d/MoltenVK_icd.json)
set(VK_LAYER_PATH    /Users/andrei/libs/VulkanSDK/macOS/share/vulkan/explicit_layer.d)
set(VULKAN_DIR         /Users/andrei/libs/VulkanSDK/macOS/)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}include/)
set(VULKAN_LIBRARY     ${VULKAN_DIR}lib/)

target_include_directories(uxn-on-gpu PRIVATE ${VULKAN_INCLUDE_DIR})
target_link_libraries(uxn-on-gpu PRIVATE ${VULKAN_LIBRARY}libvulkan.dylib ${VULKAN_LIBRARY}libvulkan.1.3.290.dylib)

# GLFW
find_package(glfw3 3.4 REQUIRED)
target_link_libraries(uxn-on-gpu PRIVATE glfw)

# GLM
find_package(glm REQUIRED)
target_link_libraries(uxn-on-gpu PRIVATE glm)

# vulkan bootstrap: this might be useful
#include(FetchContent)
#FetchContent_Declare(
#        fetch_vk_bootstrap
#        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
#        GIT_TAG        "v1.3.301"
#)
#FetchContent_MakeAvailable(fetch_vk_bootstrap)
#target_link_libraries(uxn-on-gpu PRIVATE vk-bootstrap::vk-bootstrap)

# STB
set(STB_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/external/stb/)
target_include_directories(uxn-on-gpu PUBLIC ${STB_INCLUDE_PATH})

# output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
