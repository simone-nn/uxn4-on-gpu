# uxn4-on-gpu

This project is an extension of [uxn-on-gpu](https://github.com/AndreiGhita2002/uxn-on-gpu.git), a implementation of [Uxn](https://wiki.xxiivv.com/site/uxn.html) VM that runs on almost entirely on the GPU.

Changes to the original implementation focused on:
1. Improving performance by adjusting structure design
2. Bug Fixes
3. Introducing the Parallelism API, implemented as a Uxn device


## Compilation

### Prerequisites

You will need the following installed on your system:

- **CMake** (version 3.29 or newer)
- **A C++20 compiler** (GCC, Clang, or MSVC)
- **Vulkan SDK**
    - Linux: Install via package manager (e.g. `libvulkan-dev`)
    - Windows/macOS: [Download from LunarG](https://vulkan.lunarg.com/sdk/home)
- **GLFW** (>= 3.4)
- **GLM**
- **glslc** (part of the Vulkan SDK, used to compile shaders)

### Steps

```bash
# Clone the repo
git clone https://github.com/simone-nn/uxn4-on-gpu.git
cd uxn4-on-gpu

# Create a build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build the project
cmake --build .
```

## Usage:
``uxn-on-gpu [-dm] <filename>``

- `<filename>` - Uxn .rom file you want to run inside the VM. 
There is a great selection of programs found on the internet in the `/uxn-programs/` directory.
Recommended examples: ``snake.rom`` and ``dvd.rom``.
- `-d` - enable debug more; additional print-outs for internal operations.
- `-m` - enable performance metrics; calculates average FPS, minimum and maximum frame time as well as total program duration. 

Make sure you check the README inside `uxn-programs` as not all programs are yet supported by the VM!

## The Parallelism API

- Layout: ctrl (1 byte) | lower bound (2 bytes) | upper bound (2 bytes) | invocation ID (1 byte).
- Dispatch invocations: in bilt.comp, reaplce `xxx` in `layout (local_size_x = xxx, local_size_y = 1, local_size_z = 1) in;` with the desired number of invocations. Upper limit see the hardware's `maxComputeWorkGroupInvocations` via `vulkaninfo`.
- Usage: 
    - Write to the port using `DEO/DEO2` and read from the port using `DEI/DEI2`.
    - ctrl bits: `0` for off, `1` for on with stack copy, `3` for on with empty stack.
    - Global loop bound should be written to `lower` and `upper` before setting up `ctrl` bit. The global loop is split up among worker invocations after the parallelisation start. Workers need to read back from `lower` and `upper` to retrieve their local loop bound.
    - `id` is populated after parallelisation start too. It can be used for addressing invocation local variables in the shared RAM space
- Programs: modified example Uxn programs that uses the Parallelism API see `uxn-programs/Parallelisation`. `README.md` inside the folder explains more details.

- Example:
```
|d0 @Parallel &ctrl $1 &lower $2 &upper $2 &id $1

	#0000 DUP2 .Screen/x DEO2
	.Screen/y DEO2
	[ LIT2 80 -Screen/pixel ] DEO
	;sprite/length LDA2 #0000
    .Parallel/lower DEO2 .Parallel/upper DEO2 
    #03 .Parallel/ctrl DEO .Parallel/upper DEI2 .Parallel/lower DEI2
	&loop ( -- )
		EQU2k ?&bail
		DUP2 <draw-bunny>
		INC2 !&loop
	&bail ( -- )
		POP2 POP2 
        #00 .Parallel/ctrl DEO
        BRK
```

