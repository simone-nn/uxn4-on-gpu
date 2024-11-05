#!/bin/sh

cd shaders || exit

#glslc shader.vert -o vert.spv
#glslc shader.frag -o frag.spv
echo "No shaders to compile! Edit script to add shaders."

cd ..
echo "Shaders compiled successfully!"
