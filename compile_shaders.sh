#!/bin/sh

cd shaders || exit

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader.comp -o comp.spv
echo "Note: compute shader not ready to be compiled!"

cd ..
echo "Shaders compiled successfully!"
