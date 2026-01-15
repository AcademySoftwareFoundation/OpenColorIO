# Vulkan Testing Guide for OpenColorIO PR #2243

This guide provides step-by-step instructions for testing the Vulkan unit test framework on different platforms.

## Prerequisites

Before testing the Vulkan branch, you need to install:
1. Vulkan SDK
2. glslang (for GLSL to SPIR-V compilation)
3. Standard OpenColorIO build dependencies

## Installation Instructions

### macOS

```bash
# 1. Install Vulkan SDK
# Download from: https://vulkan.lunarg.com/sdk/home
# Or use Homebrew:
brew install vulkan-sdk

# 2. Install glslang
brew install glslang

# 3. Set environment variables (add to ~/.zshrc or ~/.bash_profile)
export VULKAN_SDK=/usr/local/share/vulkan
export PATH=$VULKAN_SDK/bin:$PATH
export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d

# 4. Verify installation
vulkaninfo --summary
glslangValidator --version
```

### Linux (Ubuntu/Debian)

```bash
# 1. Install Vulkan SDK
# Option A: Using package manager
sudo apt-get update
sudo apt-get install -y vulkan-tools libvulkan-dev vulkan-validationlayers

# Option B: Download from LunarG
# Visit: https://vulkan.lunarg.com/sdk/home#linux
# Download and install the appropriate package for your distribution

# 2. Install glslang
sudo apt-get install -y glslang-tools libglslang-dev

# 3. Verify installation
vulkaninfo --summary
glslangValidator --version
```

### Linux (Fedora/RHEL/CentOS)

```bash
# 1. Install Vulkan SDK
sudo dnf install -y vulkan-tools vulkan-loader-devel vulkan-validation-layers

# 2. Install glslang
sudo dnf install -y glslang glslang-devel

# 3. Verify installation
vulkaninfo --summary
glslangValidator --version
```

### Windows

```powershell
# 1. Install Vulkan SDK
# Download from: https://vulkan.lunarg.com/sdk/home#windows
# Run the installer and follow the prompts

# 2. Install glslang (included with Vulkan SDK)
# The Vulkan SDK includes glslang, so no separate installation needed

# 3. Set environment variables (the installer should do this automatically)
# Verify these are set:
# VULKAN_SDK = C:\VulkanSDK\<version>
# PATH includes %VULKAN_SDK%\Bin

# 4. Verify installation
vulkaninfo --summary
glslangValidator --version
```

## Building OpenColorIO with Vulkan Support

### Clone and Checkout the Branch

```bash
# Clone the repository (if you haven't already)
git clone https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cd OpenColorIO

# Add pmady's fork as a remote
git remote add pmady https://github.com/pmady/OpenColorIO.git

# Fetch the branch
git fetch pmady

# Checkout the Vulkan branch
git checkout pmady/vulkan-unit-tests
# Or if you prefer to create a local branch:
git checkout -b test-vulkan-support pmady/vulkan-unit-tests
```

### Build Configuration

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake (enable Vulkan support)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOCIO_BUILD_TESTS=ON \
    -DOCIO_VULKAN_ENABLED=ON \
    -DCMAKE_INSTALL_PREFIX=../install

# Note: If CMake can't find glslang, you may need to specify:
# -Dglslang_DIR=/path/to/glslang/lib/cmake/glslang

# Build
cmake --build . --config Release -j$(nproc)

# Install (optional)
cmake --build . --target install
```

### macOS-Specific Build Notes

```bash
# On macOS, you might need to specify the Vulkan SDK path explicitly:
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOCIO_BUILD_TESTS=ON \
    -DOCIO_VULKAN_ENABLED=ON \
    -DVULKAN_SDK=/usr/local/share/vulkan \
    -Dglslang_DIR=/usr/local/lib/cmake/glslang \
    -DCMAKE_INSTALL_PREFIX=../install
```

### Windows-Specific Build Notes

```powershell
# Use Visual Studio generator
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DOCIO_BUILD_TESTS=ON ^
    -DOCIO_VULKAN_ENABLED=ON ^
    -DCMAKE_INSTALL_PREFIX=../install

# Build
cmake --build . --config Release
```

## Running the Vulkan Unit Tests

### Basic Test Execution

```bash
# Navigate to the build directory
cd build

# Run GPU unit tests with Vulkan
./tests/gpu/ocio_gpu_test --vulkan

# Run with verbose output
./tests/gpu/ocio_gpu_test --vulkan --verbose

# Run specific test
./tests/gpu/ocio_gpu_test --vulkan --gtest_filter=GPURenderer.simple_transform
```

### Verify Vulkan is Working

```bash
# Check if Vulkan device is detected
vulkaninfo | grep "deviceName"

# Run a simple Vulkan test to ensure the driver works
# (This is a separate validation step)
vkcube  # If available, shows a spinning cube
```

### Common Test Commands

```bash
# Run all GPU tests with Vulkan
./tests/gpu/ocio_gpu_test --vulkan

# Run with specific test filter
./tests/gpu/ocio_gpu_test --vulkan --gtest_filter="*transform*"

# Run with different log levels
./tests/gpu/ocio_gpu_test --vulkan --log-level=debug

# Compare Vulkan vs OpenGL results (if OpenGL is available)
./tests/gpu/ocio_gpu_test --opengl > opengl_results.txt
./tests/gpu/ocio_gpu_test --vulkan > vulkan_results.txt
diff opengl_results.txt vulkan_results.txt
```

## Troubleshooting

### Issue: CMake can't find Vulkan

**Solution:**
```bash
# Ensure VULKAN_SDK environment variable is set
echo $VULKAN_SDK  # Should show path to Vulkan SDK

# If not set, export it:
export VULKAN_SDK=/path/to/vulkan/sdk

# Or specify in CMake:
cmake .. -DVULKAN_SDK=/path/to/vulkan/sdk
```

### Issue: CMake can't find glslang

**Solution:**
```bash
# Find where glslang is installed
find /usr -name "glslangConfig.cmake" 2>/dev/null
# Or on macOS:
find /usr/local -name "glslangConfig.cmake" 2>/dev/null

# Specify the path in CMake:
cmake .. -Dglslang_DIR=/path/to/glslang/lib/cmake/glslang
```

### Issue: Vulkan tests fail with "No Vulkan device found"

**Solution:**
```bash
# Check if Vulkan is properly installed
vulkaninfo

# On Linux, you might need to install mesa-vulkan-drivers:
sudo apt-get install mesa-vulkan-drivers

# On macOS, ensure MoltenVK is properly configured:
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json
```

### Issue: Tests crash or hang

**Solution:**
```bash
# Enable Vulkan validation layers for debugging
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation

# Run with validation
./tests/gpu/ocio_gpu_test --vulkan

# Check for validation errors in the output
```

### Issue: SPIR-V compilation errors

**Solution:**
```bash
# Verify glslang is working
echo "#version 450
void main() {}" > test.comp
glslangValidator -V test.comp -o test.spv

# If this fails, reinstall glslang
```

## Expected Test Output

When running successfully, you should see output similar to:

```
[==========] Running X tests from Y test suites.
[----------] Global test environment set-up.
[----------] Z tests from GPURenderer
[ RUN      ] GPURenderer.simple_transform
Vulkan device: <Your GPU Name>
[       OK ] GPURenderer.simple_transform (XX ms)
...
[==========] X tests from Y test suites ran. (XXX ms total)
[  PASSED  ] X tests.
```

## Validation Steps

1. **Verify Vulkan SDK Installation:**
   ```bash
   vulkaninfo --summary
   ```

2. **Verify glslang Installation:**
   ```bash
   glslangValidator --version
   ```

3. **Verify Build Configuration:**
   ```bash
   cd build
   cmake -L | grep VULKAN
   # Should show: OCIO_VULKAN_ENABLED:BOOL=ON
   ```

4. **Verify Test Binary:**
   ```bash
   ls -lh tests/gpu/ocio_gpu_test
   # Should exist and be executable
   ```

5. **Run Tests:**
   ```bash
   ./tests/gpu/ocio_gpu_test --vulkan
   ```

## Platform-Specific Notes

### macOS
- Uses MoltenVK for Vulkan support
- Requires macOS 10.15+ for full Vulkan support
- Some Vulkan features may be limited compared to native implementations

### Linux
- Best native Vulkan support
- Requires proper GPU drivers (NVIDIA, AMD, or Intel)
- Headless testing works well in CI environments

### Windows
- Requires Windows 10+ with updated GPU drivers
- Visual Studio 2019 or later recommended
- May need to run as Administrator for first-time setup

## CI/CD Testing

For automated testing in CI environments:

```bash
# Install dependencies (Ubuntu example)
sudo apt-get install -y \
    vulkan-tools \
    libvulkan-dev \
    vulkan-validationlayers \
    glslang-tools \
    libglslang-dev

# Build and test
mkdir build && cd build
cmake .. -DOCIO_BUILD_TESTS=ON -DOCIO_VULKAN_ENABLED=ON
cmake --build . -j$(nproc)
./tests/gpu/ocio_gpu_test --vulkan --gtest_output=xml:test_results.xml
```

## Additional Resources

- [Vulkan SDK Documentation](https://vulkan.lunarg.com/doc/sdk)
- [glslang GitHub Repository](https://github.com/KhronosGroup/glslang)
- [OpenColorIO Documentation](https://opencolorio.readthedocs.io/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)

## Getting Help

If you encounter issues:
1. Check the troubleshooting section above
2. Verify all prerequisites are installed correctly
3. Comment on PR #2243 with specific error messages
4. Include your platform, Vulkan SDK version, and build configuration

---

**Note:** This guide is specific to testing PR #2243. For general OpenColorIO build instructions, refer to the main project documentation.
