# MAZE II

A simple arcade-like game written in C++ using Vulkan for graphics, OpenAL for audio, and GLFW for windowing.

Currently the main target is x64 Linux.

## Prerequisites

Before building, you need to install the following system dependencies:

**Tools:**
- CMake 3.25 or later
- GCC or Clang with C++23 support
- glslc

**Libs:**
- Vulkan
- OpenAL 1.24+
- FreeType 2.13+

on Ubuntu/Debian run:

```bash
sudo apt-get update
sudo apt-get install -y \
    libvulkan-dev \
    libfreetype-dev \
    libopenal-dev \
    glslc \
    libwayland-dev \
    wayland-protocols \
    libxkbcommon-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev
```

## Building

1. **Initialize Git Submodules** (required on first build):
   ```bash
   git submodule update --init --recursive
   ```

2. **Create Build Directory** (out-of-source builds required):
   ```bash
   mkdir -p build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```
   
   Optional build flags:
   - `-DNGN_ENABLE_GRAPHICS_DEBUG_LAYER=ON` - Enable graphics debug layers
   - `-DNGN_ENABLE_VISUAL_DEBUGGING=ON` - Enable visual debugging display
   - `-DCMAKE_BUILD_TYPE=Release` - Release build
   - `-DCMAKE_BUILD_TYPE=Debug` - Debug build

4. **Build the Project**:
   ```bash
   cmake --build . -j$(nproc)
   ```

5. **Run the Game**:
   ```bash
   ./src/maze/maze
   ```

## License

See [LICENSE](LICENSE) file for details.

### Third-Party Attributions

This project uses the following third-party libraries and assets:

- **STB libraries** (stb_image and stb_vorbis) - Public domain image loader and audio decoder
  - Located in `src/ext/`
  - https://github.com/nothings/stb

- **Shoot'em Up asset pack** by timberlate007
  - https://timberlate007.itch.io/shootem-up
