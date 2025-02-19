#!/bin/bash

OUTPUT_DIR="cmake-build-debug"
SHADER_DIR="shaders"
UXN_SHADERS="uxn_emu blit"

cd ..
mkdir -p ./${OUTPUT_DIR}/shaders
cd ${SHADER_DIR} || exit

for shader in $UXN_SHADERS; do
  echo "Compiling $shader"
    # Compile
    glslangValidator -V --target-env vulkan1.2 "${shader}".comp -o "${shader}".spv

    # Patch the shader
    spirv-as  --target-env vulkan1.2 <(sed -f shader_patch.sed <(spirv-dis "${shader}".spv)) -o "${shader}".spv

    # storing the spir-v disassembly
    spirv-dis "${shader}".spv -o "${shader}".spv.txt

    # Copy shader to binary directory
    cp "${shader}".spv ../${OUTPUT_DIR}/shaders/uxn_emu.spv
done

cd ..

echo "Shaders compiled successfully!"
