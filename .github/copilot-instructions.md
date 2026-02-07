# MazeII - Copilot Coding Agent Instructions

## Project Overview

**MazeII** is a simple arcade like game written in C++ using Vulkan for graphics, OpenAL for audio, and GLFW for windowing. It uses C++20 features but is compiled in C++23 mode to enhance securty. It's built with CMake and ninja.
It's primariy target platform is x64 Linux. It should be possiilbe to compile for other platforms and architectures with some modifications.

- **Repository Size**: ~700MB (with submodules)
- **Language**: C++23
- **Build System**: CMake 3.25+ required
- **Target Platform**: Linux (tested on Ubuntu 24.04)
- **Architecture**: x86-64
- **Compiler**: GCC 13.3.0+ or Clang with C++23 support
- **Graphics**: Vulkan API
- **Audio**: OpenAL
- **Windowing**: GLFW
- **Dependencies Management**: Git submodules + system packages

## Critical Build Information

### Build Prerequisites (System Dependencies)

These packages MUST be installed before building:

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

**Package Versions**:
- Freetype: 2.13+ required
- OpenAL: 1.24+ required
- Vulkan: Any modern version
- glslc: Required for shader compilation

### Build Process

**ALWAYS follow these steps in order:**

1. **Initialize Git Submodules** (REQUIRED on first build):
   ```bash
   git submodule update --init --recursive
   ```
   This downloads: CLI11, EnTT, GLFW, GLM, spdlog

2. **Create Build Directory** (out-of-source builds enforced):
   ```bash
   mkdir -p build
   cd build
   ```
   NOTE: In-source builds are explicitly prevented by CMake configuration.

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```
   
   Additional build options:
   - `-DNGN_ENABLE_GRAPHICS_DEBUG_LAYER=ON` - Enable Vulkan/OpenGL debug layers
   - `-DNGN_ENABLE_VISUAL_DEBUGGING=ON` - Enable visual display of some internal state of the engine
   - `-DNGN_ENABLE_INSTRUMENTATION=ON` - Enable performance measurement (not to be used in regular builds)
   - `-DCMAKE_BUILD_TYPE=Release` - Release build (default)
   - `-DCMAKE_BUILD_TYPE=Debug` - Debug build

4. **Build the Project**:
   ```bash
   cmake --build . -j$(nproc)
   ```
   Expected build time: ~2 minutes on modern hardware
   
   **IMPORTANT**: The first build may fail with shader compilation errors if the `src/ngn/assets/shader` directory doesn't exist. If this happens:
   ```bash
   mkdir -p src/ngn/assets/shader
   cmake --build . -j$(nproc)
   ```

5. **Run the Game**:
   ```bash
   ./src/maze/maze
   ```

6. **Run Testbed**:
   ```bash
   ./src/testbed/testbed
   ```

### Clean Rebuild

To perform a clean rebuild:
```bash
rm -rf build
mkdir build
cd build
cmake ..
mkdir -p src/ngn/assets/shader  # Prevent shader compilation failure
cmake --build . -j$(nproc)
```

## Project Architecture

### Directory Structure

```
MazeII/
├── .github/              # GitHub configuration (workflows would go here)
├── cmake/                # CMake helper modules
│   ├── CompilerWarnings.cmake    # Compiler warning configuration
│   ├── LoadCLI11.cmake           # CLI11 dependency loader
│   ├── LoadEnTT.cmake            # EnTT ECS library loader
│   ├── LoadFreetype.cmake        # Freetype font library
│   ├── LoadGLFW.cmake            # GLFW windowing library
│   ├── LoadGLM.cmake             # GLM math library
│   ├── LoadGLSLC.cmake           # Shader compiler integration
│   ├── LoadOpenAL.cmake          # OpenAL audio library
│   ├── LoadSpdlog.cmake          # Spdlog logging library
│   ├── LoadVulkan.cmake          # Vulkan graphics API
│   ├── Optimizations.cmake       # IPO/LTO settings
│   ├── PreventInSourceBuilds.cmake  # Enforces out-of-source builds
│   ├── SimdSupport.cmake         # AVX2 SIMD enablement
│   └── TargetAssets.cmake        # Asset bundling system
├── src/
│   ├── ext/              # Third-party library wrappers (STB image/vorbis)
│   ├── maze/             # Main game executable source
│   ├── ngn/              # Game engine library
│   └── testbed/          # Testing/debugging executable
├── third_party/          # Git submodules (CLI11, EnTT, GLFW, GLM, spdlog)
├── utils/
│   └── assetc/           # Asset compiler utility
├── CMakeLists.txt        # Root CMake configuration
└── README.md
```

### Key Source Components

**src/maze/** - Main Game (executable: `build/src/maze/maze`)
- `Main.cpp` - Entry point
- `MazeDelegate.*` - Game initialization and lifecycle
- `GameStage.*` - Main game logic
- `Level.*` - Level generation and management
- `Enemies.*`, `Shots.*`, `Explosions.*` - Game entities
- `assets/` - Game assets (textures, sounds, fonts)

**src/ngn/** - Game Engine Library (static library: `build/src/ngn/libngn.a`)
- `Application.*` - Application framework
- `gfx/` - Graphics subsystem (Vulkan renderer, sprites, UI)
  - `Renderer.*` - Main Vulkan renderer
  - `SpriteRenderer.*`, `UiRenderer.*` - Specialized renderers
  - `assets/shader/` - GLSL shaders (compiled to SPIR-V)
- `audio/` - Audio subsystem (OpenAL)
- `phys/` - Physics engine (collision detection, dynamics)
- `utils/` - Utility classes

**src/testbed/** - Testing Application (executable: `build/src/testbed/testbed`)

**utils/assetc/** - Asset Compiler (executable: `build/utils/assetc/assetc`)
- Compiles game assets into C++ code for embedding

### Build System Details

**CMake Configuration**:
- C++23 standard required
- Precompiled headers enabled (`Pch.hpp` files)
- AVX2 SIMD instructions enabled
- Link-time optimization (LTO/IPO) enabled in Release builds
- Strict compiler warnings enabled (see `cmake/CompilerWarnings.cmake`)

**Asset Pipeline**:
1. GLSL shaders → glslc → SPIR-V bytecode (`.spv` files)
2. Assets (textures, audio, shaders) → assetc → C++ code with embedded data
3. Generated files: `*-assets.cpp` in build directories

**Shader Compilation** (automatic during build):
- Location: `src/ngn/assets/shader/*.{vert,frag,geom}`
- Compiled with `glslc` to SPIR-V format
- Output: `build/src/ngn/assets/shader/*.spv`

### Configuration Files

- `.gitignore` - Excludes `/build/`, `/.temp/`, `/.qtcreator/`, `/CMakeLists.txt.user*`
- `.gitmodules` - Defines third-party dependencies as submodules
- `CMakeLists.txt` - Root build configuration

## Testing

**No automated tests are configured.** The project uses CTest infrastructure but defines no tests:
```bash
cd build
ctest --show-only  # Shows: "Total Tests: 0"
```

To test changes:
1. Build successfully (see Build Process above)
2. Run the executables manually:
   - `./src/maze/maze` - Main game
   - `./src/testbed/testbed` - Test application

## Common Issues and Solutions

### Issue: "CMakeLists.txt file not found" in third_party
**Solution**: Run `git submodule update --init --recursive`

### Issue: Missing system packages (Freetype, OpenAL, Vulkan)
**Solution**: Install all system dependencies listed above

### Issue: "glslc: error: cannot open output file" with "No such file or directory"
**Cause**: Missing shader output directory
**Solution**: `mkdir -p build/src/ngn/assets/shader` then rebuild

### Issue: Compilation error in World.cpp about 'aabb' not declared
**Cause**: Known bug in code (see Known Build Issue section)
**Solution**: Use `-DNGN_ENABLE_VISUAL_DEBUGGING=ON` flag

### Issue: Large files error when pushing (PCH files exceed GitHub limit)
**Cause**: Build artifacts in commits
**Solution**: Ensure `/build/` is in `.gitignore` and run `git rm -r --cached build`

## Validation Pipeline

Currently there are **no GitHub Actions workflows** or CI/CD configured. When modifying the project:

1. **Build validation**: Ensure clean build succeeds
2. **Runtime validation**: Run both executables (maze, testbed)
3. **No automated linting**: Project uses compiler warnings for code quality

## Code Patterns and Conventions

- **Entity-Component System**: Uses EnTT library for game entity management
- **Precompiled Headers**: Each module has a `Pch.hpp` - include common headers there
- **Asset Embedding**: Assets are compiled into executables, not loaded at runtime
- **Logging**: Uses spdlog library (`ngn/Logging.hpp`)
- **Math**: Uses GLM library for vectors/matrices
- **Memory**: Custom allocators in `ngn/Allocators.hpp`

## Important Notes for Coding Agents

1. **ALWAYS install system dependencies before building** - The build will fail without them
2. **ALWAYS initialize submodules first** - Third-party code is not in the repository
3. **ALWAYS use out-of-source builds** - Create a `build/` directory; in-source builds are blocked
4. **ALWAYS use the `-DNGN_ENABLE_VISUAL_DEBUGGING=ON` flag** until the aabb bug is fixed
5. **NEVER commit build artifacts** - `/build/` directory is in .gitignore
6. **Shader directory creation**: May need `mkdir -p build/src/ngn/assets/shader` on first build
7. **No tests to run**: Validation must be done by running the executables manually
8. **Trust these instructions**: The build process has been validated and documented based on actual execution. Only search for additional information if these instructions are incomplete or incorrect.
