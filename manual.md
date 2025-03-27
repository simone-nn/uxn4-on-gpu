# Uxn VM on GPU

This project is an implementation of the [Uxn](https://wiki.xxiivv.com/site/uxn.html] virtual)
machine that runs almost completely on the GPU!

More precisely, the main Uxn runtime runs on a compute shader which communicates directly with
the graphical pipeline.

## Compilation

### Prerequisites

You will need the following installed on your system:

- **CMake** (version 3.29 or newer)
- **A C++20 compiler** (GCC, Clang, or MSVC)
- **Vulkan SDK**
    - macOS: `brew install vulkan-sdk`
    - Linux: Install via package manager (e.g. `libvulkan-dev`)
    - Windows: [Download from LunarG](https://vulkan.lunarg.com/sdk/home)
- **GLFW** (>= 3.4)
- **GLM**
- **glslc** (part of the Vulkan SDK, used to compile shaders)

### Steps

```bash
# Clone the repo
git clone https://github.com/yourusername/uxn-on-gpu.git
cd uxn-on-gpu

# Create a build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build the project
cmake --build .
```

# Usage:
``uxn-on-gpu [-dm] <filename>``

- `<filename>` - Uxn .rom file you want to run inside the VM.
  There is a great selection of programs found on the internet in the `/uxn-programs/` directory.
  Recommended examples: ``snake.rom`` and ``dvd.rom``.
- `-d` - enable debug more; additional print-outs for internal operations.
- `-m` - enable performance metrics; calculates average FPS, minimum and maximum frame time as well as total program duration.

Make sure you check the README inside `uxn-programs` as not all programs are yet supported by the VM!

# Bundled programs
These are some bundled programs taken from various sources.
Programs that were not created by me or contain piece of code from others
will credit the source inside the `.tal` file.
Programs are grouped by how well they function on this implementation of the VM.
All programs are functional on the [100R](https://100r.co/site/uxn.html) runtime.

- `.tal` uxntal source code file
- `.rom` compiled uxn assembly ready to be loaded up in the VM

## Functional
- `auto` tests the Screen/auto port.
- `corners` displays sprite approximately in the windows corners.
- `dvd` the classic bouncing dvd program
- `image` displays some art
- `pixel` terrible first uxn program I wrote; it displays some coloured pixels
- `snake` the classic snake game on uxn
- `spacing` type in a string and it will print it out spaced out
- `unit_test` tests for various instructions that were tricky to implement

## Semi Functional
- `bunnymark` benchmarking program; is very glitchy
- `fib` calculates the fibonacci series. Gives weird results even on the official runtime.
- `mouse` tests mouse support. Does not render properly for some reason
- `xo` tic tac toe. Board displays but nothing else seems to work

## Not Function
- `cube3d` should render a 3D cube
