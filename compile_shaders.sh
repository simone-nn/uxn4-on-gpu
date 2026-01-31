#!/bin/bash

SHADER_DIR="shaders"
UXN_SHADERS="uxn_emu blit"
GRAPHICS_SHADERS="shader.vert shader.frag"

#cd ..
cd ${SHADER_DIR} || exit

for shader in $UXN_SHADERS; do
  echo "Compiling $shader"
  # Compile
  glslangValidator -V --target-env vulkan1.2 "${shader}".comp -o "${shader}".spv

  # # Patch the shader
  # spirv-as  --target-env vulkan1.2 <(sed -f shader_patch.sed <(spirv-dis "${shader}".spv)) -o "${shader}".spv

  # storing the spir-v disassembly
  #spirv-dis "${shader}".spv -o "${shader}".spv.txt
done

for shader in $GRAPHICS_SHADERS; do
  echo "Compiling $shader.glsl"
  # Compiling
  glslangValidator -V --target-env vulkan1.2 "${shader}".glsl -o "${shader}".spv
done

cd ..

xxd -i shaders/shader.vert.spv > src/shaders/vert.h
xxd -i shaders/shader.frag.spv > src/shaders/frag.h
xxd -i shaders/uxn_emu.spv > src/shaders/uxn_emu.h
xxd -i shaders/blit.spv > src/shaders/blit.h

echo "Shaders compiled successfully!"
