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

    explicit VulkanBuilder(VkDevice device, VkPhysicalDevice physicalDevice, 
                           VkCommandPool commandPool, VkQueue queue);
    ~VulkanBuilder();

    // Build compute shader from OCIO GpuShaderDesc.
    void buildShader(GpuShaderDescRcPtr & shaderDesc);

    // Get the compiled shader module.
    VkShaderModule getShaderModule() const { return m_shaderModule; }

    // Get the shader source code (for debugging).
    const std::string & getShaderSource() const { return m_shaderSource; }

    // Allocate and setup all textures (3D LUTs and 1D/2D LUTs).
    void allocateAllTextures(GpuShaderDescRcPtr & shaderDesc);

    // Get descriptor set layout bindings for textures and uniforms.
    std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayoutBindings() const;

    // Get descriptor pool sizes for textures and uniforms.
    std::vector<VkDescriptorPoolSize> getDescriptorPoolSizes() const;

    // Update descriptor set with texture and uniform bindings.
    void updateDescriptorSet(VkDescriptorSet descriptorSet);

    // Update uniform values before each dispatch.
    void updateUniforms();

    // Get the uniform buffer for binding.
    VkBuffer getUniformBuffer() const { return m_uniformBuffer; }
    VkDeviceSize getUniformBufferSize() const { return m_uniformBufferSize; }

    // Check if uniforms are used.
    bool hasUniforms() const { return m_uniformBufferSize > 0; }

    // Check if textures are used.
    bool hasTextures() const { return !m_textures3D.empty() || !m_textures1D2D.empty(); }

private:
    // Compile GLSL to SPIR-V.
    std::vector<uint32_t> compileGLSLToSPIRV(const std::string & glslSource);

    // Helper to find memory type.
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Helper to create image.
    void createImage(uint32_t width, uint32_t height, uint32_t depth,
                     VkFormat format, VkImageType imageType,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage & image, VkDeviceMemory & imageMemory);

    // Helper to create image view.
    VkImageView createImageView(VkImage image, VkFormat format, VkImageViewType viewType);

    // Helper to create sampler.
    VkSampler createSampler(Interpolation interpolation);

    // Helper to transition image layout.
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    // Helper to copy buffer to image.
    void copyBufferToImage(VkBuffer buffer, VkImage image,
                           uint32_t width, uint32_t height, uint32_t depth);

    // Create uniform buffer.
    void createUniformBuffer(GpuShaderDescRcPtr & shaderDesc);

    // Delete all resources.
    void deleteAllTextures();
    void deleteUniformBuffer();

    VkDevice m_device{ VK_NULL_HANDLE };
    VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
    VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    VkQueue m_queue{ VK_NULL_HANDLE };
    VkShaderModule m_shaderModule{ VK_NULL_HANDLE };
    std::string m_shaderSource;
    GpuShaderDescRcPtr m_shaderDesc;

    // Texture resources for 3D LUTs
    struct TextureResource
    {
        VkImage image{ VK_NULL_HANDLE };
        VkDeviceMemory memory{ VK_NULL_HANDLE };
        VkImageView imageView{ VK_NULL_HANDLE };
        VkSampler sampler{ VK_NULL_HANDLE };
        std::string samplerName;
        uint32_t binding{ 0 };
    };
    std::vector<TextureResource> m_textures3D;
    std::vector<TextureResource> m_textures1D2D;

    // Uniform buffer for dynamic parameters
    VkBuffer m_uniformBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory m_uniformBufferMemory{ VK_NULL_HANDLE };
    VkDeviceSize m_uniformBufferSize{ 0 };

    // Uniform data structure matching shader layout
    struct UniformData
    {
        std::string name;
        GpuShaderDesc::UniformData data;
        size_t offset;
        size_t size;
    };
    std::vector<UniformData> m_uniforms;
};

} // namespace OCIO_NAMESPACE

#endif // OCIO_VULKAN_ENABLED

#endif // INCLUDED_OCIO_VULKANAPP_H
