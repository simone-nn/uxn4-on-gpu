#!/bin/sh

cd shaders || exit

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader.comp -o comp.spv
glslangValidator -V --target-env vulkan1.2 uxn_emu.comp -o uxn_emu.spv
#glslangValidator -V --target-env vulkan1.2 simple.comp -o simple.spv

cd ..
echo "Shaders compiled successfully!"
