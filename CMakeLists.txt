cmake_minimum_required(VERSION 3.20)

project(MazeII LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# include basic helpers
include(PreventInSourceBuilds)
include(CMakeDependentOption)

# build options
option(NGN_ENABLE_GRAPHICS_DEBUG_LAYER "Enable debug layers in opengl and vulkan" OFF)
option(NGN_ENABLE_VISUAL_DEBUGGING "Enable building visual debugging components" OFF)

# include more helpers
include(CompilerWarnings)
include(Optimizations)
include(SimdSupport)
include(TargetAssets)

# global compile options
enable_ipo()

# compile options for own targets
add_library(compile_options INTERFACE)
target_compile_features(compile_options INTERFACE cxx_std_20)
set_project_warnings(compile_options)
enable_avx2(compile_options)

# dependencies
include(LoadCLI11)
include(LoadFreetype)
include(LoadGLFW)
include(LoadGLM)
include(LoadGLSLC)
include(LoadSpdlog)
include(LoadVulkan)

# dig into sources
add_subdirectory(src)
add_subdirectory(utils)
