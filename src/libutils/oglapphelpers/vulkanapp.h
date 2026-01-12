// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_VULKANAPP_H
#define INCLUDED_OCIO_VULKANAPP_H

#ifdef OCIO_VULKAN_ENABLED

#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

class VulkanBuilder;
typedef OCIO_SHARED_PTR<VulkanBuilder> VulkanBuilderRcPtr;

class VulkanApp;
typedef OCIO_SHARED_PTR<VulkanApp> VulkanAppRcPtr;

// VulkanApp provides headless Vulkan rendering for GPU unit testing.
// This class is designed to process images using OCIO GPU shaders via Vulkan compute pipelines.
class VulkanApp
{
public:
    VulkanApp() = delete;
    VulkanApp(const VulkanApp &) = delete;
    VulkanApp & operator=(const VulkanApp &) = delete;

    // Initialize the app with given buffer size for headless rendering.
    VulkanApp(int bufWidth, int bufHeight);

    virtual ~VulkanApp();

    enum Components
    {
        COMPONENTS_RGB = 0,
        COMPONENTS_RGBA
    };

    // Initialize the image buffer.
    void initImage(int imageWidth, int imageHeight, Components comp, const float * imageBuffer);

    // Update the image if it changes.
    void updateImage(const float * imageBuffer);

    // Set the shader code from OCIO GpuShaderDesc.
    void setShader(GpuShaderDescRcPtr & shaderDesc);

    // Update the size of the buffer used to process the image.
    void reshape(int width, int height);

    // Process the image using the Vulkan compute pipeline.
    void redisplay();

    // Read the processed image from the GPU buffer.
    void readImage(float * imageBuffer);

    // Print Vulkan device and instance info.
    void printVulkanInfo() const noexcept;

    // Factory method to create a VulkanApp instance.
    static VulkanAppRcPtr CreateVulkanApp(int bufWidth, int bufHeight);

    // Shader code will be printed when generated.
    void setPrintShader(bool print) { m_printShader = print; }

protected:
    // Initialize Vulkan instance, device, and queues.
    void initVulkan();

    // Create Vulkan compute pipeline for shader processing.
    void createComputePipeline();

    // Create buffers for image data.
    void createBuffers();

    // Clean up Vulkan resources.
    void cleanup();

    // Helper to find a suitable memory type.
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Helper to create a Vulkan buffer.
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, VkBuffer & buffer, 
                      VkDeviceMemory & bufferMemory);

    // Helper to copy buffer data.
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
    // Vulkan core objects
    VkInstance m_instance{ VK_NULL_HANDLE };
    VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
    VkDevice m_device{ VK_NULL_HANDLE };
    VkQueue m_computeQueue{ VK_NULL_HANDLE };
    uint32_t m_computeQueueFamilyIndex{ 0 };

    // Command pool and buffer
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };

    // Compute pipeline
    VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    VkPipeline m_computePipeline{ VK_NULL_HANDLE };
    VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
    VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };

    // Shader module
    VkShaderModule m_computeShaderModule{ VK_NULL_HANDLE };

    // Image buffers
    VkBuffer m_inputBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory m_inputBufferMemory{ VK_NULL_HANDLE };
    VkBuffer m_outputBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory m_outputBufferMemory{ VK_NULL_HANDLE };
    VkBuffer m_stagingBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory m_stagingBufferMemory{ VK_NULL_HANDLE };

    // Image dimensions
    int m_imageWidth{ 0 };
    int m_imageHeight{ 0 };
    int m_bufferWidth{ 0 };
    int m_bufferHeight{ 0 };
    Components m_components{ COMPONENTS_RGBA };

    // Shader builder
    VulkanBuilderRcPtr m_vulkanBuilder;

    // Debug and configuration
    bool m_printShader{ false };
    bool m_initialized{ false };

    // Validation layers (debug builds)
#ifdef NDEBUG
    const bool m_enableValidationLayers{ false };
#else
    const bool m_enableValidationLayers{ true };
#endif
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};

// VulkanBuilder handles OCIO shader compilation for Vulkan.
class VulkanBuilder
{
public:
    VulkanBuilder() = delete;
    VulkanBuilder(const VulkanBuilder &) = delete;
    VulkanBuilder & operator=(const VulkanBuilder &) = delete;

    explicit VulkanBuilder(VkDevice device);
    ~VulkanBuilder();

    // Build compute shader from OCIO GpuShaderDesc.
    void buildShader(GpuShaderDescRcPtr & shaderDesc);

    // Get the compiled shader module.
    VkShaderModule getShaderModule() const { return m_shaderModule; }

    // Get the shader source code (for debugging).
    const std::string & getShaderSource() const { return m_shaderSource; }

    // Allocate and setup 3D LUT textures.
    void allocateAllTextures(unsigned maxTextureSize);

    // Get descriptor set layout bindings for textures.
    std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayoutBindings() const;

    // Update descriptor set with texture bindings.
    void updateDescriptorSet(VkDescriptorSet descriptorSet);

private:
    // Compile GLSL to SPIR-V.
    std::vector<uint32_t> compileGLSLToSPIRV(const std::string & glslSource);

    VkDevice m_device{ VK_NULL_HANDLE };
    VkShaderModule m_shaderModule{ VK_NULL_HANDLE };
    std::string m_shaderSource;

    // Texture resources for 3D LUTs
    struct TextureResource
    {
        VkImage image{ VK_NULL_HANDLE };
        VkDeviceMemory memory{ VK_NULL_HANDLE };
        VkImageView imageView{ VK_NULL_HANDLE };
        VkSampler sampler{ VK_NULL_HANDLE };
    };
    std::vector<TextureResource> m_textures;
};

} // namespace OCIO_NAMESPACE

#endif // OCIO_VULKAN_ENABLED

#endif // INCLUDED_OCIO_VULKANAPP_H
