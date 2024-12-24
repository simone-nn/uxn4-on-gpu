#!/bin/sh

cd shaders || exit

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader.comp -o comp.spv

cd ..
echo "Shaders compiled successfully!"
