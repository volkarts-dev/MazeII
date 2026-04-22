# MazeII - Copilot Coding Agent Instructions

## Project Overview

**MazeII** is a simple arcade-like game written in C++ using Vulkan for graphics, OpenAL for audio, and GLFW for windowing. It is compiled in C++23 mode. It's built with CMake (Ninja generator recommended).
Its primary target platform is x64 Linux. It should be possible to compile for other platforms and architectures with some modifications.

- **Repository Size**: ~700MB (with submodules)
- **Language**: C++23
- **Build System**: CMake
- **Target Platform**: Linux (tested on Debian 13)
- **Architecture**: x86-64
- **Compiler**: GCC or Clang with C++23 support
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
- CMake 3.25+
- GCC 14+ or Clang 19+
- Freetype: 2.13+
- OpenAL: 1.24+
- Vulkan: 1.1+
- glslc: 15+

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
   
   Additional build options (all default to `OFF`, defined in the root `CMakeLists.txt`):
   - `-DNGN_ENABLE_GRAPHICS_DEBUG_LAYER=ON` - Enable debug layers in Vulkan/OpenGL
   - `-DNGN_ENABLE_VISUAL_DEBUGGING=ON` - Enable visual display of some internal state of the engine (e.g. physics AABBs)
   - `-DNGN_ENABLE_INSTRUMENTATION=ON` - Enable performance measurement (not to be used in regular builds)
   - `-DNGN_ENABLE_DEVELOPER_HACKS=ON` - Enable some hacks to support development
   - `-DCMAKE_BUILD_TYPE=Release` - Release build (default)
   - `-DCMAKE_BUILD_TYPE=Debug` - Debug build

4. **Build the Project**:
   ```bash
   cmake --build .
   ```
   Expected build time: ~2 minutes on modern hardware
   
   ```bash
   mkdir -p src/ngn/assets/shader
   cmake --build .
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
cmake --build .
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
- `MazeDelegate.*` - Game initialization and lifecycle (application delegate)
- `GameStage.*` - Main in-game stage/logic
- `Board.*` - Playfield/maze board
- `Player.*`, `Enemies.*`, `Shots.*`, `Explosions.*` - Game systems
- `MazeComponents.hpp`, `Layers.hpp` - ECS components and rendering/physics layer definitions
- `gfx/` - Game-specific UI (`Dialog`, `OverviewMap`)
- `assets/` - Game assets (textures, sounds, fonts)
- `MazeAssets.hpp.in` - Asset manifest template processed by `assetc`

**src/ngn/** - Game Engine Library (static library: `build/src/ngn/libngn.a`)
- `Application.*` - Application framework / main loop
- `Timer.*`, `Logging.*`, `Instrumentation.*`, `Allocators.*` - Core utilities
- `Input.hpp`, `Math.hpp`, `Types.hpp`, `Macros.hpp`, `CommonComponents.hpp`
- `gfx/` - Graphics subsystem (Vulkan)
  - `Renderer.*` - Main Vulkan renderer
  - `SpriteRenderer.*` / `SpritePipeline.*` - Sprite rendering
  - `DebugRenderer.*` / `DebugPipeline.*` - Visual debug drawing (guarded by `NGN_ENABLE_VISUAL_DEBUGGING`)
  - `FontCollection.*`, `FontMaker.*` - Freetype-based font handling
  - `Buffer.*`, `Image.*`, `Pipeline.*`, `CommandBuffer.*`, `RenderTarget.*` - Vulkan wrappers
  - `SpriteAnimator.*`, `SpriteAnimation.hpp` - Animation
- `audio/` - Audio subsystem (`Audio`, `AudioBuffer`, `Sound`) on OpenAL
- `phys/` - Physics engine (`World`, `Solver`, `DynamicTree`, `Shapes`, `Collision*`, `Intersection*`, `Functions`, `Layers`)
- `ai/` - AI helpers (`NavigationGraph`, `SteeringHelper`)
- `ext/` - Third-party library wrappers (`StbImage`, `StbVorbis`)
- `utils/` - Utility containers (`Array`, `StaticVector`)
- `assets/shader/` - GLSL shaders (`Sprite.{vert,geom,frag}`, `Debug.{vert,frag}`) compiled to SPIR-V
- `Assets.hpp.in` - Engine asset manifest template

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

### Issue: Large files error when pushing (PCH files exceed GitHub limit)
**Cause**: Build artifacts in commits
**Solution**: Ensure `/build/` is in `.gitignore` and run `git rm -r --cached build`

## Validation Pipeline

There is **no build/test CI**. When modifying the project:

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
4. **NEVER commit build artifacts** - `/build/` directory is in .gitignore
5. **No tests to run**: Validation must be done by running the executables manually
6. **Trust these instructions**: The build process has been validated and documented based on actual execution. Only search for additional information if these instructions are incomplete or incorrect.
