# CodeQL Workflow Documentation

## Overview

This repository includes a CodeQL Advanced workflow for automated security scanning of C++ code. CodeQL is GitHub's semantic code analysis engine that helps identify security vulnerabilities and coding errors.

## Workflow Configuration

The CodeQL workflow is configured in `.github/workflows/codeql.yml` and runs:

### Automatic Triggers
- **On push to main branch**: Scans code whenever changes are pushed to main
- **On pull requests to main**: Scans code in pull requests before merging

### Manual Trigger (On-Demand)
You can manually trigger the CodeQL analysis at any time:

1. Go to the **Actions** tab in the GitHub repository
2. Select **CodeQL Advanced** from the workflows list
3. Click **Run workflow** button
4. Select the branch you want to analyze
5. Click **Run workflow** to start the analysis

## Build Configuration

The workflow uses a **manual build mode** with the following setup:

### System Dependencies
The following packages are installed to support the build:
- CMake and Ninja build system
- OpenGL development libraries (libgl1-mesa-dev, libglu1-mesa-dev)
- X11 development libraries (libx11-dev, libxrandr-dev, libxinerama-dev, libxcursor-dev, libxi-dev)
- Wayland development libraries (libwayland-dev, libwayland-bin, libxkbcommon-dev)
- Audio libraries (libopenal-dev)
- Font rendering (libfreetype6-dev)
- Vulkan SDK (libvulkan-dev)
- GLSL compiler (glslc)

### Git Submodules
The workflow automatically initializes git submodules recursively, which are required for third-party dependencies:
- CLI11
- EnTT
- GLFW
- GLM
- spdlog

### Build Process
The workflow:
1. Creates a `build` directory
2. Configures the project using CMake with Ninja generator and Release build type
3. Builds the project using CMake
4. CodeQL analyzes the compiled code

## Language Support

Currently configured for:
- **C/C++** (c-cpp)

## Viewing Results

After a CodeQL analysis completes:
1. Go to the **Security** tab in the GitHub repository
2. Click on **Code scanning** in the left sidebar
3. View the alerts and findings from CodeQL

## Troubleshooting

### Build Failures
If the CodeQL workflow fails during the build step:
- Check the workflow logs in the Actions tab
- Verify that all dependencies are correctly installed
- Ensure git submodules are properly initialized

### Analysis Failures
If CodeQL analysis fails:
- Check that the build completed successfully
- Review the CodeQL initialization logs
- Verify the language matrix configuration is correct

## Customization

To customize the analysis:
- Add custom queries by uncommenting the `queries` parameter in the workflow
- Modify the build commands in the "Run manual build steps" section
- Add additional languages to the matrix if needed

## References

- [CodeQL Documentation](https://codeql.github.com/docs/)
- [Code Scanning Documentation](https://docs.github.com/en/code-security/code-scanning)
- [CodeQL for C/C++](https://docs.github.com/en/code-security/code-scanning/creating-an-advanced-setup-for-code-scanning/codeql-code-scanning-for-compiled-languages)
