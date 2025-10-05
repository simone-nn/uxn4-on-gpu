cmake_minimum_required(VERSION 3.29)
project(uxn-on-gpu)

set(CMAKE_CXX_STANDARD 20)


# === Shader headers and SPIR-V outputs ===
set(SHADER_HEADERS
        ${CMAKE_SOURCE_DIR}/src/shaders/uxn_emu.h
        ${CMAKE_SOURCE_DIR}/src/shaders/blit.h
        ${CMAKE_SOURCE_DIR}/src/shaders/vert.h
        ${CMAKE_SOURCE_DIR}/src/shaders/frag.h
)

set(SHADER_SPV
        ${CMAKE_SOURCE_DIR}/shaders/uxn_emu.spv
        ${CMAKE_SOURCE_DIR}/shaders/blit.spv
        ${CMAKE_SOURCE_DIR}/shaders/shader.vert.spv
        ${CMAKE_SOURCE_DIR}/shaders/shader.frag.spv
)

# Shader compilation
add_custom_command(
        OUTPUT ${SHADER_HEADERS} ${SHADER_SPV}
        COMMAND ${CMAKE_SOURCE_DIR}/compile_shaders.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS
        ${CMAKE_SOURCE_DIR}/shaders/uxn_emu.comp
        ${CMAKE_SOURCE_DIR}/shaders/blit.comp
        ${CMAKE_SOURCE_DIR}/shaders/shader.vert.glsl
        ${CMAKE_SOURCE_DIR}/shaders/shader.frag.glsl
        BYPRODUCTS ${SHADER_HEADERS} ${SHADER_SPV}
        COMMENT "Compiling shaders..."
)
# Define a target for shader compilation
add_custom_target(compile_shaders
        DEPENDS ${SHADER_HEADERS} ${SHADER_SPV}
)

# === Main executable ===
add_executable(uxn-on-gpu
        ${CMAKE_SOURCE_DIR}/src/shaders/blit.h
        ${CMAKE_SOURCE_DIR}/src/shaders/frag.h
        ${CMAKE_SOURCE_DIR}/src/shaders/uxn_emu.h
        ${CMAKE_SOURCE_DIR}/src/shaders/vert.h
        ${CMAKE_SOURCE_DIR}/src/Resource.hpp
        ${CMAKE_SOURCE_DIR}/src/DeviceController.hpp
        ${CMAKE_SOURCE_DIR}/src/Uxn.hpp
        ${CMAKE_SOURCE_DIR}/src/DeviceController.cpp
        ${CMAKE_SOURCE_DIR}/src/Resource.cpp
        ${CMAKE_SOURCE_DIR}/src/Uxn.cpp
        ${CMAKE_SOURCE_DIR}/src/Console.cpp
        ${CMAKE_SOURCE_DIR}/src/Console.hpp
        ${CMAKE_SOURCE_DIR}/src/Io.cpp
        ${CMAKE_SOURCE_DIR}/src/Io.hpp
        ${CMAKE_SOURCE_DIR}/src/FpsLogger.cpp
        ${CMAKE_SOURCE_DIR}/src/FpsLogger.hpp
)

add_dependencies(uxn-on-gpu compile_shaders)

# === Vulkan (portable detection) ===
find_package(Vulkan REQUIRED)
target_link_libraries(uxn-on-gpu PRIVATE Vulkan::Vulkan)

# === GLFW ===
find_package(glfw3 3.4 REQUIRED)
target_link_libraries(uxn-on-gpu PRIVATE glfw)

# === GLM ===
find_package(glm REQUIRED)
target_link_libraries(uxn-on-gpu PRIVATE glm::glm)

# === Output dirs ===
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
