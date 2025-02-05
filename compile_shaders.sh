#!/bin/sh

cd shaders || exit

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader.comp -o comp.spv
glslc uxn_emu.comp -o uxn_emu.spv
#glslc simple.comp -o simple.spv
glslangValidator -V --target-env vulkan1.3 simple.comp -o simple.spv

cd ..
echo "Shaders compiled successfully!"
