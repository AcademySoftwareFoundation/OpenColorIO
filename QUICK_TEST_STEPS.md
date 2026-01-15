# Quick Testing Steps for PR #2243

Hi @doug-walker! Here are the quick steps to test the Vulkan branch:

## Quick Start (macOS)

```bash
# 1. Install dependencies
brew install vulkan-sdk glslang

# 2. Set environment variables
export VULKAN_SDK=/usr/local/share/vulkan
export PATH=$VULKAN_SDK/bin:$PATH
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json

# 3. Verify installation
vulkaninfo --summary
glslangValidator --version

# 4. Clone and build
git clone https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cd OpenColorIO
git remote add pmady https://github.com/pmady/OpenColorIO.git
git fetch pmady
git checkout pmady/vulkan-unit-tests

mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOCIO_BUILD_TESTS=ON \
    -DOCIO_VULKAN_ENABLED=ON \
    -Dglslang_DIR=/usr/local/lib/cmake/glslang

cmake --build . -j$(sysctl -n hw.ncpu)

# 5. Run tests
./tests/gpu/ocio_gpu_test --vulkan
```

## Quick Start (Linux - Ubuntu/Debian)

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y vulkan-tools libvulkan-dev vulkan-validationlayers \
    glslang-tools libglslang-dev mesa-vulkan-drivers

# 2. Verify installation
vulkaninfo --summary
glslangValidator --version

# 3. Clone and build
git clone https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cd OpenColorIO
git remote add pmady https://github.com/pmady/OpenColorIO.git
git fetch pmady
git checkout pmady/vulkan-unit-tests

mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOCIO_BUILD_TESTS=ON \
    -DOCIO_VULKAN_ENABLED=ON

cmake --build . -j$(nproc)

# 4. Run tests
./tests/gpu/ocio_gpu_test --vulkan
```

## Quick Start (Windows)

```powershell
# 1. Download and install Vulkan SDK from:
# https://vulkan.lunarg.com/sdk/home#windows

# 2. Verify installation (in PowerShell)
vulkaninfo --summary
glslangValidator --version

# 3. Clone and build
git clone https://github.com/AcademySoftwareFoundation/OpenColorIO.git
cd OpenColorIO
git remote add pmady https://github.com/pmady/OpenColorIO.git
git fetch pmady
git checkout pmady/vulkan-unit-tests

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_TESTS=ON -DOCIO_VULKAN_ENABLED=ON
cmake --build . --config Release

# 4. Run tests
.\tests\gpu\Release\ocio_gpu_test.exe --vulkan
```

## Common Issues

**CMake can't find glslang:**
```bash
# Find glslang location
find /usr -name "glslangConfig.cmake" 2>/dev/null

# Add to cmake command:
cmake .. -Dglslang_DIR=/path/to/glslang/lib/cmake/glslang ...
```

**No Vulkan device found:**
```bash
# Check Vulkan installation
vulkaninfo

# On Linux, install GPU drivers:
sudo apt-get install mesa-vulkan-drivers  # For Intel/AMD
# Or install NVIDIA/AMD proprietary drivers
```

**Tests crash:**
```bash
# Enable validation layers for debugging
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
./tests/gpu/ocio_gpu_test --vulkan
```

## Expected Output

```
[==========] Running tests...
[----------] Tests from GPURenderer
[ RUN      ] GPURenderer.simple_transform
Vulkan device: <Your GPU Name>
[       OK ] GPURenderer.simple_transform
...
[  PASSED  ] All tests
```

For detailed instructions, see the full [VULKAN_TESTING_GUIDE.md](./VULKAN_TESTING_GUIDE.md).

Let me know if you run into any issues!
