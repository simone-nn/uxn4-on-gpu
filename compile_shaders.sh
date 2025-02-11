#!/bin/bash

OUTPUT_DIR="cmake-build-debug"
SHADER_DIR="shaders"

cd ../${SHADER_DIR} || exit

#glslc shader.vert -o vert.spv
#glslc shader.frag -o frag.spv

# Compile
glslangValidator -V --target-env vulkan1.2 uxn_emu.comp -o uxn_emu.spv

# Patch the shader
spirv-as <(sed -f shader_patch.sed <(spirv-dis uxn_emu.spv)) -o uxn_emu.spv

cd ..

# Copy shader to binary directory
mkdir -p ./${OUTPUT_DIR}/shaders
cp ./${SHADER_DIR}/uxn_emu.spv ./${OUTPUT_DIR}/shaders/uxn_emu.spv

echo "Shaders compiled successfully!"
