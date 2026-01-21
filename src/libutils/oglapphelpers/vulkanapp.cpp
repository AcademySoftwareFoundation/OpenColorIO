// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifdef OCIO_VULKAN_ENABLED

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <fstream>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "vulkanapp.h"

namespace OCIO_NAMESPACE
{

//
// VulkanApp Implementation
//

VulkanApp::VulkanApp(int bufWidth, int bufHeight)
    : m_bufferWidth(bufWidth)
    , m_bufferHeight(bufHeight)
{
    initVulkan();
}

VulkanApp::~VulkanApp()
{
    cleanup();
}

void VulkanApp::initVulkan()
{
    // Create Vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "OCIO GPU Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "OCIO";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Required extensions for MoltenVK on macOS
    std::vector<const char*> extensions;
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (m_enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    // Select physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Find a device with compute queue support
    for (const auto & device : devices)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_physicalDevice = device;
                m_computeQueueFamilyIndex = i;
                break;
            }
        }

        if (m_physicalDevice != VK_NULL_HANDLE)
        {
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU with compute support");
    }

    // Print GPU information for diagnostics
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    std::cout << "Vulkan GPU: " << deviceProperties.deviceName << std::endl;
    std::cout << "  Device Type: ";
    switch (deviceProperties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   std::cout << "Discrete GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: std::cout << "Integrated GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    std::cout << "Virtual GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            std::cout << "CPU (Software)"; break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
        default:                                     std::cout << "Other"; break;
    }
    std::cout << std::endl;
    std::cout << "  API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
              << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
              << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;

    // Create logical device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_computeQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 0;

    if (m_enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    vkGetDeviceQueue(m_device, m_computeQueueFamilyIndex, 0, &m_computeQueue);

    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_computeQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool");
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    m_initialized = true;
}

void VulkanApp::cleanup()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);

        // Destroy VulkanBuilder first (it holds shader module references)
        m_vulkanBuilder.reset();

        if (m_computePipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        }
        if (m_descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        }
        if (m_descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        }
        if (m_computeShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_computeShaderModule, nullptr);
        }

        if (m_inputBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, m_inputBuffer, nullptr);
        }
        if (m_inputBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_inputBufferMemory, nullptr);
        }
        if (m_outputBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, m_outputBuffer, nullptr);
        }
        if (m_outputBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_outputBufferMemory, nullptr);
        }
        if (m_stagingBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
        }
        if (m_stagingBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_stagingBufferMemory, nullptr);
        }

        if (m_commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        }

        vkDestroyDevice(m_device, nullptr);
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
    }
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties, VkBuffer & buffer,
                              VkDeviceMemory & bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_computeQueue);

    vkResetCommandBuffer(m_commandBuffer, 0);
}

void VulkanApp::initImage(int imageWidth, int imageHeight, Components comp, const float * imageBuffer)
{
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    m_components = comp;

    createBuffers();
    updateImage(imageBuffer);
}

void VulkanApp::createBuffers()
{
    const int numComponents = (m_components == COMPONENTS_RGB) ? 3 : 4;
    const VkDeviceSize bufferSize = m_imageWidth * m_imageHeight * numComponents * sizeof(float);

    // Create staging buffer (CPU accessible)
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_stagingBuffer, m_stagingBufferMemory);

    // Create input buffer (GPU only)
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_inputBuffer, m_inputBufferMemory);

    // Create output buffer (GPU only)
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_outputBuffer, m_outputBufferMemory);
}

void VulkanApp::updateImage(const float * imageBuffer)
{
    const int numComponents = (m_components == COMPONENTS_RGB) ? 3 : 4;
    const VkDeviceSize bufferSize = m_imageWidth * m_imageHeight * numComponents * sizeof(float);

    // Copy data to staging buffer
    void * data;
    vkMapMemory(m_device, m_stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, imageBuffer, static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, m_stagingBufferMemory);

    // Copy from staging to input buffer
    copyBuffer(m_stagingBuffer, m_inputBuffer, bufferSize);
}

void VulkanApp::setShader(GpuShaderDescRcPtr & shaderDesc)
{
    if (!m_vulkanBuilder)
    {
        m_vulkanBuilder = std::make_shared<VulkanBuilder>(m_device, m_physicalDevice, 
                                                           m_commandPool, m_computeQueue);
    }

    // Allocate textures and uniforms before building shader
    m_vulkanBuilder->allocateAllTextures(shaderDesc);
    m_vulkanBuilder->buildShader(shaderDesc);

    if (m_printShader)
    {
        std::cout << "Vulkan Compute Shader:\n" << m_vulkanBuilder->getShaderSource() << std::endl;
    }

    createComputePipeline();
}

void VulkanApp::createComputePipeline()
{
    // Create descriptor set layout
    // Use high binding numbers (100, 101) for input/output buffers to avoid conflicts with OCIO bindings
    // OCIO uses binding 0 for uniforms and 1+ for textures
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // Input buffer binding
        {100, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Output buffer binding
        {101, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };

    // Add texture and uniform bindings from shader builder
    auto textureBindings = m_vulkanBuilder->getDescriptorSetLayoutBindings();
    bindings.insert(bindings.end(), textureBindings.begin(), textureBindings.end());

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = m_vulkanBuilder->getShaderModule();
    pipelineInfo.stage.pName = "main";

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    // Create descriptor pool with sizes from VulkanBuilder
    std::vector<VkDescriptorPoolSize> poolSizes = m_vulkanBuilder->getDescriptorPoolSizes();

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    // Update descriptor set with buffer bindings
    const int numComponents = (m_components == COMPONENTS_RGB) ? 3 : 4;
    const VkDeviceSize bufferSize = m_imageWidth * m_imageHeight * numComponents * sizeof(float);

    VkDescriptorBufferInfo inputBufferInfo{};
    inputBufferInfo.buffer = m_inputBuffer;
    inputBufferInfo.offset = 0;
    inputBufferInfo.range = bufferSize;

    VkDescriptorBufferInfo outputBufferInfo{};
    outputBufferInfo.buffer = m_outputBuffer;
    outputBufferInfo.offset = 0;
    outputBufferInfo.range = bufferSize;

    // Use high binding numbers (100, 101) to avoid conflicts with OCIO bindings
    // OCIO uses binding 0 for uniforms and 1+ for textures
    std::vector<VkWriteDescriptorSet> descriptorWrites = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, 100, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &inputBufferInfo, nullptr},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, 101, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &outputBufferInfo, nullptr}
    };

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);

    // Update texture and uniform bindings
    m_vulkanBuilder->updateDescriptorSet(m_descriptorSet);
}

void VulkanApp::reshape(int width, int height)
{
    m_bufferWidth = width;
    m_bufferHeight = height;
}

void VulkanApp::redisplay()
{
    // Update uniform values before dispatch (for dynamic parameters)
    if (m_vulkanBuilder)
    {
        m_vulkanBuilder->updateUniforms();
    }

    // Record command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    // Dispatch compute shader
    const uint32_t groupCountX = (m_imageWidth + 15) / 16;
    const uint32_t groupCountY = (m_imageHeight + 15) / 16;
    vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, 1);

    vkEndCommandBuffer(m_commandBuffer);

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_computeQueue);

    vkResetCommandBuffer(m_commandBuffer, 0);
}

void VulkanApp::readImage(float * imageBuffer)
{
    const int numComponents = (m_components == COMPONENTS_RGB) ? 3 : 4;
    const VkDeviceSize bufferSize = m_imageWidth * m_imageHeight * numComponents * sizeof(float);

    // Copy from output buffer to staging buffer
    copyBuffer(m_outputBuffer, m_stagingBuffer, bufferSize);

    // Read from staging buffer
    void * data;
    vkMapMemory(m_device, m_stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(imageBuffer, data, static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, m_stagingBufferMemory);
}

void VulkanApp::printVulkanInfo() const noexcept
{
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        std::cout << "Vulkan not initialized" << std::endl;
        return;
    }

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    std::cout << "Vulkan Device: " << properties.deviceName << std::endl;
    std::cout << "Vulkan API Version: "
              << VK_VERSION_MAJOR(properties.apiVersion) << "."
              << VK_VERSION_MINOR(properties.apiVersion) << "."
              << VK_VERSION_PATCH(properties.apiVersion) << std::endl;
    std::cout << "Driver Version: " << properties.driverVersion << std::endl;
}

VulkanAppRcPtr VulkanApp::CreateVulkanApp(int bufWidth, int bufHeight)
{
    return std::make_shared<VulkanApp>(bufWidth, bufHeight);
}

//
// VulkanBuilder Implementation
//

VulkanBuilder::VulkanBuilder(VkDevice device, VkPhysicalDevice physicalDevice,
                             VkCommandPool commandPool, VkQueue queue)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_commandPool(commandPool)
    , m_queue(queue)
{
}

VulkanBuilder::~VulkanBuilder()
{
    deleteAllTextures();
    deleteUniformBuffer();

    if (m_device != VK_NULL_HANDLE && m_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    }
}

void VulkanBuilder::deleteAllTextures()
{
    if (m_device == VK_NULL_HANDLE) return;

    auto deleteTextures = [this](std::vector<TextureResource> & textures) {
        for (auto & tex : textures)
        {
            if (tex.sampler != VK_NULL_HANDLE)
                vkDestroySampler(m_device, tex.sampler, nullptr);
            if (tex.imageView != VK_NULL_HANDLE)
                vkDestroyImageView(m_device, tex.imageView, nullptr);
            if (tex.image != VK_NULL_HANDLE)
                vkDestroyImage(m_device, tex.image, nullptr);
            if (tex.memory != VK_NULL_HANDLE)
                vkFreeMemory(m_device, tex.memory, nullptr);
        }
        textures.clear();
    };

    deleteTextures(m_textures3D);
    deleteTextures(m_textures1D2D);
}

void VulkanBuilder::deleteUniformBuffer()
{
    if (m_device == VK_NULL_HANDLE) return;

    if (m_uniformBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }
    if (m_uniformBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);
        m_uniformBufferMemory = VK_NULL_HANDLE;
    }
    m_uniformBufferSize = 0;
    m_uniforms.clear();
}

uint32_t VulkanBuilder::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanBuilder::createImage(uint32_t width, uint32_t height, uint32_t depth,
                                 VkFormat format, VkImageType imageType,
                                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                 VkImage & image, VkDeviceMemory & imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);
}

VkImageView VulkanBuilder::createImageView(VkImage image, VkFormat format, VkImageViewType viewType)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image view");
    }
    return imageView;
}

VkSampler VulkanBuilder::createSampler(Interpolation interpolation)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    
    if (interpolation == INTERP_NEAREST)
    {
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
    }
    else
    {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
    }
    
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create sampler");
    }
    return sampler;
}

void VulkanBuilder::transitionImageLayout(VkImage image, VkFormat /*format*/,
                                           VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else
    {
        throw std::runtime_error("Unsupported layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanBuilder::copyBufferToImage(VkBuffer buffer, VkImage image,
                                       uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, depth};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanBuilder::createUniformBuffer(GpuShaderDescRcPtr & shaderDesc)
{
    deleteUniformBuffer();

    const unsigned numUniforms = shaderDesc->getNumUniforms();
    if (numUniforms == 0) return;

    // Use OCIO's provided buffer size and offsets - these are calculated correctly
    // for the scalar layout used in Vulkan shaders
    m_uniformBufferSize = shaderDesc->getUniformBufferSize();
    if (m_uniformBufferSize == 0) return;

    // Store uniform metadata using OCIO's provided offsets
    for (unsigned idx = 0; idx < numUniforms; ++idx)
    {
        GpuShaderDesc::UniformData data;
        const char * name = shaderDesc->getUniform(idx, data);
        
        UniformData uniformData;
        uniformData.name = name;
        uniformData.data = data;
        uniformData.offset = data.m_bufferOffset; // Use OCIO's calculated offset
        
        // Calculate size based on type (for debugging/verification)
        if (data.m_getDouble)
        {
            uniformData.size = sizeof(float);
        }
        else if (data.m_getBool)
        {
            uniformData.size = sizeof(int);
        }
        else if (data.m_getFloat3)
        {
            uniformData.size = 3 * sizeof(float);
        }
        else if (data.m_vectorFloat.m_getSize)
        {
            // Size will be determined when writing data
            uniformData.size = 0;
        }
        else if (data.m_vectorInt.m_getSize)
        {
            // Size will be determined when writing data
            uniformData.size = 0;
        }
        else
        {
            std::cerr << "Warning: Unknown uniform type for '" << name << "'" << std::endl;
            continue;
        }
        
        m_uniforms.push_back(uniformData);
    }

    // Create uniform buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_uniformBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_uniformBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create uniform buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_uniformBufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate uniform buffer memory");
    }

    vkBindBufferMemory(m_device, m_uniformBuffer, m_uniformBufferMemory, 0);

    // Initialize uniform values
    updateUniforms();
}

void VulkanBuilder::updateUniforms()
{
    if (m_uniformBufferSize == 0 || m_uniformBuffer == VK_NULL_HANDLE || !m_shaderDesc) return;

    void * data;
    vkMapMemory(m_device, m_uniformBufferMemory, 0, m_uniformBufferSize, 0, &data);
    
    // Zero-initialize the buffer first to ensure clean state
    memset(data, 0, m_uniformBufferSize);

    // Query uniform values directly from shader description (callbacks may have updated)
    const unsigned numUniforms = m_shaderDesc->getNumUniforms();
    for (unsigned idx = 0; idx < numUniforms && idx < m_uniforms.size(); ++idx)
    {
        GpuShaderDesc::UniformData uniformData;
        m_shaderDesc->getUniform(idx, uniformData);
        
        char * dest = static_cast<char*>(data) + m_uniforms[idx].offset;
        
        if (uniformData.m_getDouble)
        {
            float val = static_cast<float>(uniformData.m_getDouble());
            memcpy(dest, &val, sizeof(float));
        }
        else if (uniformData.m_getBool)
        {
            int val = uniformData.m_getBool() ? 1 : 0;
            memcpy(dest, &val, sizeof(int));
        }
        else if (uniformData.m_getFloat3)
        {
            // vec3 in std140: write 3 floats (12 bytes), padded to 16 bytes
            auto vals = uniformData.m_getFloat3();
            memcpy(dest, vals.data(), 3 * sizeof(float));
        }
        else if (uniformData.m_vectorFloat.m_getSize && uniformData.m_vectorFloat.m_getVector)
        {
            // In std140, each array element is padded to 16 bytes
            const float * vals = uniformData.m_vectorFloat.m_getVector();
            size_t count = uniformData.m_vectorFloat.m_getSize();
            for (size_t i = 0; i < count; ++i)
            {
                memcpy(dest + i * 16, &vals[i], sizeof(float));
            }
        }
        else if (uniformData.m_vectorInt.m_getSize && uniformData.m_vectorInt.m_getVector)
        {
            // In std140, each array element is padded to 16 bytes
            const int * vals = uniformData.m_vectorInt.m_getVector();
            size_t count = uniformData.m_vectorInt.m_getSize();
            for (size_t i = 0; i < count; ++i)
            {
                memcpy(dest + i * 16, &vals[i], sizeof(int));
            }
        }
    }

    vkUnmapMemory(m_device, m_uniformBufferMemory);
}

void VulkanBuilder::allocateAllTextures(GpuShaderDescRcPtr & shaderDesc)
{
    deleteAllTextures();
    m_shaderDesc = shaderDesc;

    // Create uniform buffer for dynamic parameters
    createUniformBuffer(shaderDesc);

    // Process 3D LUTs - use OCIO's get3DTextureShaderBindingIndex for correct bindings
    const unsigned num3DTextures = shaderDesc->getNum3DTextures();
    for (unsigned idx = 0; idx < num3DTextures; ++idx)
    {
        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned edgelen = 0;
        Interpolation interpolation = INTERP_LINEAR;
        shaderDesc->get3DTexture(idx, textureName, samplerName, edgelen, interpolation);

        if (!textureName || !samplerName || edgelen == 0)
        {
            throw std::runtime_error("Invalid 3D texture data");
        }

        const float * values = nullptr;
        shaderDesc->get3DTextureValues(idx, values);
        if (!values)
        {
            throw std::runtime_error("Missing 3D texture values");
        }

        // Use RGBA format since RGB32F is not widely supported (especially on MoltenVK/macOS)
        // Convert RGB to RGBA by adding alpha = 1.0
        const size_t numTexels = edgelen * edgelen * edgelen;
        std::vector<float> rgbaValues(numTexels * 4);
        for (size_t i = 0; i < numTexels; ++i)
        {
            rgbaValues[i * 4 + 0] = values[i * 3 + 0];
            rgbaValues[i * 4 + 1] = values[i * 3 + 1];
            rgbaValues[i * 4 + 2] = values[i * 3 + 2];
            rgbaValues[i * 4 + 3] = 1.0f;
        }

        // Create staging buffer
        VkDeviceSize imageSize = numTexels * 4 * sizeof(float);
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer);

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory);
        vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

        // Copy RGBA data to staging buffer
        void * data;
        vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, rgbaValues.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(m_device, stagingBufferMemory);

        // Create 3D image with RGBA format
        // Use OCIO's get3DTextureShaderBindingIndex for the correct binding index
        TextureResource tex;
        tex.samplerName = samplerName;
        tex.binding = shaderDesc->get3DTextureShaderBindingIndex(idx);

        createImage(edgelen, edgelen, edgelen, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_3D,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory);

        // Transition and copy
        transitionImageLayout(tex.image, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, tex.image, edgelen, edgelen, edgelen);
        transitionImageLayout(tex.image, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Create image view and sampler
        tex.imageView = createImageView(tex.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_3D);
        tex.sampler = createSampler(interpolation);

        m_textures3D.push_back(tex);

        // Cleanup staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    // Process 1D/2D LUTs
    const unsigned numTextures = shaderDesc->getNumTextures();
    for (unsigned idx = 0; idx < numTextures; ++idx)
    {
        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned width = 0;
        unsigned height = 0;
        GpuShaderDesc::TextureType channel = GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        Interpolation interpolation = INTERP_LINEAR;
        GpuShaderDesc::TextureDimensions dimensions = GpuShaderDesc::TEXTURE_1D;
        shaderDesc->getTexture(idx, textureName, samplerName, width, height, channel, dimensions, interpolation);

        if (!textureName || !samplerName || width == 0)
        {
            throw std::runtime_error("Invalid texture data");
        }

        const float * values = nullptr;
        shaderDesc->getTextureValues(idx, values);
        if (!values)
        {
            throw std::runtime_error("Missing texture values");
        }

        // Use R32 for single channel, RGBA32 for RGB (since RGB32F not supported on MoltenVK)
        unsigned imgHeight = (height > 0) ? height : 1;
        const size_t numTexels = width * imgHeight;
        VkFormat format;
        VkDeviceSize imageSize;
        std::vector<float> convertedValues;

        if (channel == GpuShaderDesc::TEXTURE_RED_CHANNEL)
        {
            format = VK_FORMAT_R32_SFLOAT;
            imageSize = numTexels * sizeof(float);
        }
        else
        {
            // Convert RGB to RGBA
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            imageSize = numTexels * 4 * sizeof(float);
            convertedValues.resize(numTexels * 4);
            for (size_t i = 0; i < numTexels; ++i)
            {
                convertedValues[i * 4 + 0] = values[i * 3 + 0];
                convertedValues[i * 4 + 1] = values[i * 3 + 1];
                convertedValues[i * 4 + 2] = values[i * 3 + 2];
                convertedValues[i * 4 + 3] = 1.0f;
            }
        }

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer);

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory);
        vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

        // Copy data
        void * data;
        vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
        if (channel == GpuShaderDesc::TEXTURE_RED_CHANNEL)
        {
            memcpy(data, values, static_cast<size_t>(imageSize));
        }
        else
        {
            memcpy(data, convertedValues.data(), static_cast<size_t>(imageSize));
        }
        vkUnmapMemory(m_device, stagingBufferMemory);

        // Create image (use 2D for both 1D and 2D textures in Vulkan)
        // Use OCIO's getTextureShaderBindingIndex for the correct binding index
        TextureResource tex;
        tex.samplerName = samplerName;
        tex.binding = shaderDesc->getTextureShaderBindingIndex(idx);

        createImage(width, imgHeight, 1, format, VK_IMAGE_TYPE_2D,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory);

        // Transition and copy
        transitionImageLayout(tex.image, format,
                              VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, tex.image, width, imgHeight, 1);
        transitionImageLayout(tex.image, format,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Create image view and sampler
        tex.imageView = createImageView(tex.image, format, VK_IMAGE_VIEW_TYPE_2D);
        tex.sampler = createSampler(interpolation);

        m_textures1D2D.push_back(tex);

        // Cleanup staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }
}

void VulkanBuilder::buildShader(GpuShaderDescRcPtr & shaderDesc)
{
    // Generate GLSL compute shader source from OCIO shader description
    std::ostringstream shader;

    shader << "#version 460\n";
    shader << "#extension GL_EXT_scalar_block_layout : enable\n";
    shader << "\n";
    shader << "layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n";
    shader << "\n";
    // Use high binding numbers for input/output buffers to avoid conflicts with OCIO bindings.
    // OCIO uses binding 0 for uniforms and 1+ for textures (via setDescriptorSetIndex(0, 1)).
    // By using bindings 100 and 101 for I/O buffers, we avoid needing to edit OCIO's shader text.
    shader << "layout(std430, set = 0, binding = 100) readonly buffer InputBuffer {\n";
    shader << "    vec4 inputPixels[];\n";
    shader << "};\n";
    shader << "\n";
    shader << "layout(std430, set = 0, binding = 101) writeonly buffer OutputBuffer {\n";
    shader << "    vec4 outputPixels[];\n";
    shader << "};\n";
    shader << "\n";

    // OCIO generates texture sampler declarations with correct bindings when using
    // GPU_LANGUAGE_GLSL_VK_4_6 and setDescriptorSetIndex(0, 1) is called on the shader descriptor.
    // OCIO uses binding 0 for uniforms (by design) and 1+ for textures.

    // Get OCIO shader text - it already contains sampler and uniform declarations with correct bindings
    const char * shaderText = shaderDesc->getShaderText();
    if (shaderText && strlen(shaderText) > 0)
    {
        shader << shaderText;
    }

    shader << "\n";
    shader << "void main() {\n";
    shader << "    uvec2 gid = gl_GlobalInvocationID.xy;\n";
    shader << "    uint width = 256u;\n";
    shader << "    uint height = 256u;\n";
    shader << "    \n";
    shader << "    // Bounds check to avoid out-of-bounds access\n";
    shader << "    if (gid.x >= width || gid.y >= height) return;\n";
    shader << "    \n";
    shader << "    uint idx = gid.y * width + gid.x;\n";
    shader << "    \n";
    shader << "    vec4 " << shaderDesc->getPixelName() << " = inputPixels[idx];\n";
    shader << "    \n";

    // Call the OCIO color transformation function
    const char * functionName = shaderDesc->getFunctionName();
    if (functionName && strlen(functionName) > 0)
    {
        shader << "    " << shaderDesc->getPixelName() << " = " << functionName 
               << "(" << shaderDesc->getPixelName() << ");\n";
    }

    shader << "    \n";
    shader << "    outputPixels[idx] = " << shaderDesc->getPixelName() << ";\n";
    shader << "}\n";

    m_shaderSource = shader.str();

    // Compile GLSL to SPIR-V
    std::vector<uint32_t> spirvCode = compileGLSLToSPIRV(m_shaderSource);

    // Create shader module
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }
}

std::vector<uint32_t> VulkanBuilder::compileGLSLToSPIRV(const std::string & glslSource)
{
    // Initialize glslang (safe to call multiple times)
    static bool glslangInitialized = false;
    if (!glslangInitialized)
    {
        glslang::InitializeProcess();
        glslangInitialized = true;
    }

    // Create shader object
    glslang::TShader shader(EShLangCompute);
    
    const char * shaderStrings[1] = { glslSource.c_str() };
    shader.setStrings(shaderStrings, 1);

    // Set up Vulkan 1.2 / SPIR-V 1.5 environment
    shader.setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

    // Get default resource limits
    const TBuiltInResource * resources = GetDefaultResources();

    // Parse the shader
    const int defaultVersion = 460;
    const bool forwardCompatible = false;
    const EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(resources, defaultVersion, forwardCompatible, messages))
    {
        std::string errorMsg = "GLSL parsing failed:\n";
        errorMsg += shader.getInfoLog();
        errorMsg += "\n";
        errorMsg += shader.getInfoDebugLog();
        throw std::runtime_error(errorMsg);
    }

    // Create program and link
    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages))
    {
        std::string errorMsg = "GLSL linking failed:\n";
        errorMsg += program.getInfoLog();
        errorMsg += "\n";
        errorMsg += program.getInfoDebugLog();
        throw std::runtime_error(errorMsg);
    }

    // Convert to SPIR-V
    std::vector<uint32_t> spirv;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = false;
    spvOptions.stripDebugInfo = true;
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize = false;

    glslang::GlslangToSpv(*program.getIntermediate(EShLangCompute), spirv, &spvOptions);

    if (spirv.empty())
    {
        throw std::runtime_error("SPIR-V generation produced empty output");
    }

    return spirv;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBuilder::getDescriptorSetLayoutBindings() const
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Add uniform buffer binding at binding 0 (OCIO's default)
    // OCIO generates uniform blocks with binding = 0 by design
    if (hasUniforms())
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    // Add bindings for 3D LUT textures (starting at binding 1 via setDescriptorSetIndex(0, 1))
    for (const auto & tex : m_textures3D)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = tex.binding;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    // Add bindings for 1D/2D LUT textures
    for (const auto & tex : m_textures1D2D)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = tex.binding;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    return bindings;
}

std::vector<VkDescriptorPoolSize> VulkanBuilder::getDescriptorPoolSizes() const
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    // Storage buffers for input/output
    poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2});

    // Uniform buffer for dynamic parameters
    if (hasUniforms())
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
    }

    // Combined image samplers for textures
    uint32_t numTextures = static_cast<uint32_t>(m_textures3D.size() + m_textures1D2D.size());
    if (numTextures > 0)
    {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numTextures});
    }

    return poolSizes;
}

void VulkanBuilder::updateDescriptorSet(VkDescriptorSet descriptorSet)
{
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::vector<VkDescriptorImageInfo> imageInfos;
    
    // Use a single buffer info for uniform buffer (allocated on stack to avoid reallocation issues)
    VkDescriptorBufferInfo uniformBufferInfo{};

    // Reserve space to prevent reallocation
    imageInfos.reserve(m_textures3D.size() + m_textures1D2D.size());

    // Add uniform buffer binding at binding 0 (OCIO's default)
    if (hasUniforms())
    {
        uniformBufferInfo.buffer = m_uniformBuffer;
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = m_uniformBufferSize;

        VkWriteDescriptorSet uniformWrite{};
        uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformWrite.dstSet = descriptorSet;
        uniformWrite.dstBinding = 0;
        uniformWrite.dstArrayElement = 0;
        uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformWrite.descriptorCount = 1;
        uniformWrite.pBufferInfo = &uniformBufferInfo;
        descriptorWrites.push_back(uniformWrite);
    }

    // Add 3D texture bindings (starting at binding 1 via setDescriptorSetIndex(0, 1))
    for (const auto & tex : m_textures3D)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tex.imageView;
        imageInfo.sampler = tex.sampler;
        imageInfos.push_back(imageInfo);

        VkWriteDescriptorSet imageWrite{};
        imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageWrite.dstSet = descriptorSet;
        imageWrite.dstBinding = tex.binding;
        imageWrite.dstArrayElement = 0;
        imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageWrite.descriptorCount = 1;
        imageWrite.pImageInfo = &imageInfos.back();
        descriptorWrites.push_back(imageWrite);
    }

    // Add 1D/2D texture bindings
    for (const auto & tex : m_textures1D2D)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tex.imageView;
        imageInfo.sampler = tex.sampler;
        imageInfos.push_back(imageInfo);

        VkWriteDescriptorSet imageWrite{};
        imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageWrite.dstSet = descriptorSet;
        imageWrite.dstBinding = tex.binding;
        imageWrite.dstArrayElement = 0;
        imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageWrite.descriptorCount = 1;
        imageWrite.pImageInfo = &imageInfos.back();
        descriptorWrites.push_back(imageWrite);
    }

    if (!descriptorWrites.empty())
    {
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

} // namespace OCIO_NAMESPACE

#endif // OCIO_VULKAN_ENABLED
