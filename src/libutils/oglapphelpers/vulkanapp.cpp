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
        m_vulkanBuilder = std::make_shared<VulkanBuilder>(m_device);
    }

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
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // Input buffer binding
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        // Output buffer binding
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };

    // Add texture bindings from shader builder
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

    // Create descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
    };

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

    std::vector<VkWriteDescriptorSet> descriptorWrites = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, 0, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &inputBufferInfo, nullptr},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, 1, 0, 1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &outputBufferInfo, nullptr}
    };

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);

    // Update texture bindings
    m_vulkanBuilder->updateDescriptorSet(m_descriptorSet);
}

void VulkanApp::reshape(int width, int height)
{
    m_bufferWidth = width;
    m_bufferHeight = height;
}

void VulkanApp::redisplay()
{
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

VulkanBuilder::VulkanBuilder(VkDevice device)
    : m_device(device)
{
}

VulkanBuilder::~VulkanBuilder()
{
    if (m_device != VK_NULL_HANDLE)
    {
        if (m_shaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
        }

        for (auto & tex : m_textures)
        {
            if (tex.sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(m_device, tex.sampler, nullptr);
            }
            if (tex.imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_device, tex.imageView, nullptr);
            }
            if (tex.image != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_device, tex.image, nullptr);
            }
            if (tex.memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(m_device, tex.memory, nullptr);
            }
        }
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
    shader << "layout(std430, set = 0, binding = 0) readonly buffer InputBuffer {\n";
    shader << "    vec4 inputPixels[];\n";
    shader << "};\n";
    shader << "\n";
    shader << "layout(std430, set = 0, binding = 1) writeonly buffer OutputBuffer {\n";
    shader << "    vec4 outputPixels[];\n";
    shader << "};\n";
    shader << "\n";

    // Add OCIO shader helper code
    shader << shaderDesc->getShaderText() << "\n";

    shader << "\n";
    shader << "void main() {\n";
    shader << "    uvec2 gid = gl_GlobalInvocationID.xy;\n";
    shader << "    uint width = " << 256 << ";\n";  // Will be set dynamically
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

void VulkanBuilder::allocateAllTextures(unsigned maxTextureSize)
{
    // TODO: Implement 3D LUT texture allocation for OCIO
    // This would create VkImage, VkImageView, and VkSampler for each LUT
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBuilder::getDescriptorSetLayoutBindings() const
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    // Add bindings for 3D LUT textures
    // Starting at binding index 2 (0 and 1 are for input/output buffers)
    uint32_t bindingIndex = 2;
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = bindingIndex++;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    return bindings;
}

void VulkanBuilder::updateDescriptorSet(VkDescriptorSet descriptorSet)
{
    // TODO: Update descriptor set with texture bindings
    // This would write VkDescriptorImageInfo for each LUT texture
}

} // namespace OCIO_NAMESPACE

#endif // OCIO_VULKAN_ENABLED
