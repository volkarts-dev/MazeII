# Security Scanning Workflows

## Overview

This repository uses multiple complementary security scanning tools to provide broad coverage across different vulnerability categories.

| Workflow | Tool | What it scans |
|---|---|---|
| `codeql.yml` | CodeQL Advanced | C++ semantic analysis – logic bugs and security vulnerabilities |
| `trivy.yml` | Trivy | Secrets in code, YAML/CMake misconfigurations |
| `osv-scanner.yml` | OSV-Scanner | Known CVEs in git-submodule dependencies |
| `dependency-review.yml` | Dependency Review | Vulnerable dependencies introduced by a PR |

---

## CodeQL Workflow

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

---

## Trivy Workflow

Trivy (`trivy.yml`) runs on push/PR to main.

It performs a **filesystem scan** for:
- **Secrets** – API keys, tokens, and credentials accidentally committed to source
- **Misconfigurations** – insecure settings in GitHub Actions YAML and other config files

Results are uploaded to the GitHub Security tab (SARIF format) under the `trivy` category.

---

## OSV-Scanner Workflow

OSV-Scanner (`osv-scanner.yml`) runs on push/PR to main and weekly (Monday 08:15 UTC).

It scans all git submodules recursively against the [OSV vulnerability database](https://osv.dev/), which aggregates CVEs from NVD, GitHub Advisory Database, and other sources.

Results are uploaded to the GitHub Security tab (SARIF format) under the `osv-scanner` category.

---

## Dependency Review Workflow

Dependency Review (`dependency-review.yml`) runs only on **pull requests** to main.

It compares the dependency manifest snapshot of the base and head commits and fails the check if any newly introduced dependency has a known vulnerability rated **Critical** or **High**. A summary comment is automatically posted on the PR.

---

## Viewing All Security Results

CodeQL, Trivy, and OSV-Scanner each upload SARIF results to GitHub's Security tab (Dependency Review posts its findings directly as a PR comment instead):
1. Go to the **Security** tab in the GitHub repository
2. Click **Code scanning** in the left sidebar
3. Use the **Tool** filter to switch between CodeQL, Trivy, and OSV-Scanner results
