// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "msl.h"
#include "mtltexture.h"

#import <Metal/Metal.h>

namespace OCIO_NAMESPACE
{

std::vector<float> RGB_to_RGBA(const float* lutValues, int valueCount)
{
    if(valueCount % 3 != 0)
        throw Exception("Value count should be divisible by 3.");
    
    valueCount = valueCount * 4 / 3;
    std::vector<float> float4AdaptedLutValues;
    if(lutValues != nullptr)
    {
        float4AdaptedLutValues.resize(valueCount);
        const float *rgbLutValuesIt = lutValues;
        float *rgbaLutValuesIt = float4AdaptedLutValues.data();
        const float *end = rgbaLutValuesIt + valueCount;
            
        while(rgbaLutValuesIt != end) {
            *rgbaLutValuesIt++ = *rgbLutValuesIt++;
            *rgbaLutValuesIt++ = *rgbLutValuesIt++;
            *rgbaLutValuesIt++ = *rgbLutValuesIt++;
            *rgbaLutValuesIt++ = 1.0f;
        }
    }
    
    return float4AdaptedLutValues;
}

namespace
{

id<MTLSamplerState> GetSamplerState(id<MTLDevice> device, Interpolation interpolation)
{
    static std::unordered_map<int, id<MTLSamplerState>> samplerStateCache;
    
    if(samplerStateCache.find(interpolation) != samplerStateCache.end())
        return samplerStateCache[interpolation];
    
    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor new] autorelease];
    [samplerDesc setMinFilter:MTLSamplerMinMagFilterNearest];
    
    if(interpolation==INTERP_NEAREST)
    {
        [samplerDesc setMinFilter:MTLSamplerMinMagFilterNearest];
        [samplerDesc setMagFilter:MTLSamplerMinMagFilterNearest];
    }
    else
    {
        [samplerDesc setMinFilter:MTLSamplerMinMagFilterLinear];
        [samplerDesc setMagFilter:MTLSamplerMinMagFilterLinear];
    }

    [samplerDesc setSAddressMode:MTLSamplerAddressModeClampToEdge];
    [samplerDesc setTAddressMode:MTLSamplerAddressModeClampToEdge];
    [samplerDesc setRAddressMode:MTLSamplerAddressModeClampToEdge];
    
    samplerStateCache[interpolation] = [[device newSamplerStateWithDescriptor:samplerDesc] autorelease];
    
    return samplerStateCache[interpolation];
}

id<MTLTexture> AllocateTexture3D(id<MTLDevice> device,
                       unsigned edgelen, const float * lutValues)
{
    if(lutValues==0x0)
    {
        throw Exception("Missing texture data");
    }
    
    // MTLPixelFormatRGB32Float not supported on metal. Adapt to MTLPixelFormatRGBA32Float
    std::vector<float> float4AdaptedLutValues = RGB_to_RGBA(lutValues, 3*edgelen*edgelen*edgelen);
    
    MTLTextureDescriptor* texDescriptor = [[MTLTextureDescriptor new] autorelease];
    
    [texDescriptor setTextureType:MTLTextureType3D];
    [texDescriptor setWidth:edgelen];
    [texDescriptor setHeight:edgelen];
    [texDescriptor setDepth:edgelen];
    [texDescriptor setStorageMode:MTLStorageModeManaged];
    [texDescriptor setPixelFormat:MTLPixelFormatRGBA32Float];
    [texDescriptor setMipmapLevelCount:1];
    id<MTLTexture> tex = [[device newTextureWithDescriptor:texDescriptor] autorelease];
    
    const void* texData = float4AdaptedLutValues.data();
    
    [tex replaceRegion:MTLRegionMake3D(0, 0, 0, edgelen, edgelen, edgelen)
           mipmapLevel:0
                 slice:0
             withBytes:texData
           bytesPerRow:edgelen * 4 * sizeof(float)
         bytesPerImage:edgelen * edgelen * 4 * sizeof(float)];
    
    return tex;
}

id<MTLTexture> AllocateTexture2D(id<MTLDevice> device,
                                 unsigned width, unsigned height,
                                 GpuShaderDesc::TextureType channel,
                                 const float * values)
{
    if (values == nullptr)
    {
        throw Exception("Missing texture data.");
    }
    
    const int channelPerPix = channel == GpuShaderCreator::TEXTURE_RED_CHANNEL ? 1 : 4;
    std::vector<float> adaptedLutValues;
    if(channel == GpuShaderDesc::TEXTURE_RED_CHANNEL)
    {
        adaptedLutValues.resize(width * height);
        memcpy(adaptedLutValues.data(), values, width * height * sizeof(float));
    }
    else
        adaptedLutValues = RGB_to_RGBA(values, 3*width*height);
    
    MTLTextureDescriptor* texDescriptor = [[MTLTextureDescriptor new] autorelease];
    
    MTLPixelFormat pixelFormat = channel == GpuShaderDesc::TEXTURE_RED_CHANNEL ? MTLPixelFormatR32Float : MTLPixelFormatRGBA32Float;
    
    [texDescriptor setTextureType:height > 1 ? MTLTextureType2D : MTLTextureType1D];
    [texDescriptor setWidth:width];
    [texDescriptor setHeight:height];
    [texDescriptor setDepth:1];
    [texDescriptor setStorageMode:MTLStorageModeManaged];
    [texDescriptor setPixelFormat:pixelFormat];
    [texDescriptor setMipmapLevelCount:1];
    id<MTLTexture> tex = [[device newTextureWithDescriptor:texDescriptor] autorelease];
    
    [tex replaceRegion:MTLRegionMake3D(0, 0, 0, width, height, 1) mipmapLevel:0 withBytes:adaptedLutValues.data() bytesPerRow:channelPerPix * width * sizeof(float)];
    
    return tex;
}
}

//////////////////////////////////////////////////////////

MetalBuilder::Uniform::Uniform(const std::string & name, const GpuShaderDesc::UniformData & data)
    : m_name(name)
    , m_data(data)
{
}

//////////////////////////////////////////////////////////

MetalBuilderRcPtr MetalBuilder::Create(const GpuShaderDescRcPtr & shaderDesc)
{
    return MetalBuilderRcPtr(new MetalBuilder(shaderDesc));
}

MetalBuilder::MetalBuilder(const GpuShaderDescRcPtr & shaderDesc)
    :   m_shaderDesc(shaderDesc)
    ,   m_startIndex(0)
    ,   m_uniformData{}
    ,   m_verbose(false)
{
    m_device = MTLCreateSystemDefaultDevice();
    m_cmdQueue = [[m_device newCommandQueue] autorelease];
}

MetalBuilder::~MetalBuilder()
{
}

void MetalBuilder::allocateAllTextures(unsigned startIndex)
{
    deleteAllTextures();

    // This is the first available index for the textures.
    m_startIndex = startIndex;
    unsigned currIndex = m_startIndex;

    // Process the 3D LUT first.

    const unsigned maxTexture3D = m_shaderDesc->getNum3DTextures();
    for(unsigned idx=0; idx<maxTexture3D; ++idx)
    {
        // 1. Get the information of the 3D LUT.

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned edgelen = 0;
        Interpolation interpolation = INTERP_LINEAR;
        m_shaderDesc->get3DTexture(idx, textureName, samplerName, edgelen, interpolation);

        if(!textureName || !*textureName
            || !samplerName || !*samplerName
            || edgelen==0)
        {
            throw Exception("The texture data is corrupted");
        }

        const float * values = nullptr;
        m_shaderDesc->get3DTextureValues(idx, values);
        if(!values)
        {
            throw Exception("The texture values are missing");
        }
        
        // 2. Allocate the 3D LUT.

        id<MTLTexture> texture = AllocateTexture3D(m_device, edgelen, values);
        id<MTLSamplerState> samplerState = GetSamplerState(m_device, interpolation);

        // 3. Keep the texture id & name for the later enabling.

        m_textureIds.push_back(TextureId(texture, textureName, samplerState, samplerName, MTLTextureType3D));

        currIndex++;
    }

    // Process the 1D LUTs.

    const unsigned maxTexture2D = m_shaderDesc->getNumTextures();
    for(unsigned idx=0; idx<maxTexture2D; ++idx)
    {
        // 1. Get the information of the 1D LUT.

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned width = 0;
        unsigned height = 0;
        GpuShaderDesc::TextureType channel = GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        Interpolation interpolation = INTERP_LINEAR;
        m_shaderDesc->getTexture(idx, textureName, samplerName, width, height, channel, interpolation);

        if (!textureName || !*textureName
            || !samplerName || !*samplerName
            || width==0)
        {
            throw Exception("The texture data is corrupted");
        }

        const float * values = 0x0;
        m_shaderDesc->getTextureValues(idx, values);
        if(!values)
        {
            throw Exception("The texture values are missing");
        }

        // 2. Allocate the 1D LUT (a 2D texture is needed to hold large LUTs).
        id<MTLTexture> texture = AllocateTexture2D(m_device, width, height, channel, values);
        id<MTLSamplerState> samplerState = GetSamplerState(m_device, interpolation);

        // 3. Keep the texture id & name for the later enabling.

        MTLTextureType type = (height > 1) ? MTLTextureType2D : MTLTextureType1D;
        m_textureIds.push_back(TextureId(texture, textureName, samplerState, samplerName, type));
        currIndex++;
    }
}

void MetalBuilder::deleteAllTextures()
{
    m_textureIds.clear();
}

void MetalBuilder::fillUniformBufferData()
{
    m_uniformData.clear();
    m_uniformData.reserve(1024);
    const unsigned maxUniforms = m_shaderDesc->getNumUniforms();
    
    int alignment = 4;
    for (unsigned idx = 0; idx < maxUniforms; ++idx)
    {
        GpuShaderDesc::UniformData data;
        m_shaderDesc->getUniform(idx, data);
        switch(data.m_type)
        {
            case UNIFORM_BOOL:
            {
                float v = data.m_getBool() == false ? 0.0f : 1.0f;
                size_t offset = m_uniformData.size();
                size_t dataSize = sizeof(float);
                m_uniformData.resize(offset + dataSize);
                memcpy(&m_uniformData[offset], &v, dataSize);
                alignment = std::max(alignment, 4);
            }
            break;

        case UNIFORM_DOUBLE:
            {
                float v = data.m_getDouble();
                size_t offset = m_uniformData.size();
                size_t dataSize = sizeof(float);
                m_uniformData.resize(offset + dataSize);
                memcpy(&m_uniformData[offset], &v, dataSize);
                alignment = std::max(alignment, 4);
            }
            break;

        case UNIFORM_FLOAT3:
            {
                const float* v = data.m_getFloat3().data();
                size_t offset = m_uniformData.size();
                size_t dataSize = 3 * sizeof(float);
                m_uniformData.resize(offset + 4 * sizeof(float));
                memcpy(&m_uniformData[offset], v, dataSize);
                alignment = std::max(alignment, 16);
            }
                // Part of uniform data buffer
            break;
                
            case UNIFORM_VECTOR_INT:
            {
                const int v = data.m_vectorInt.m_getSize();
                size_t offset = m_uniformData.size();
                size_t dataSize = sizeof(int);
                m_uniformData.resize(offset + dataSize);
                memcpy(&m_uniformData[offset], &v, dataSize);
                alignment = std::max(alignment, 4);
            }
            break;
                
            case UNIFORM_VECTOR_FLOAT:
            {
                const int v = data.m_vectorFloat.m_getSize();
                size_t offset = m_uniformData.size();
                size_t dataSize = sizeof(int);
                m_uniformData.resize(offset + dataSize);
                memcpy(&m_uniformData[offset], &v, dataSize);
                alignment = std::max(alignment, 4);
            }
            break;
                
            case UNIFORM_UNKNOWN:
                throw Exception("Unknown uniform type.");
        };
    }
    
    m_uniformData.resize(((m_uniformData.size() + alignment - 1) / alignment) * alignment);
}

void MetalBuilder::setUniforms(id<MTLRenderCommandEncoder> renderCmdEncoder)
{
    fillUniformBufferData();
    if(m_uniformData.size() > 0)
        [renderCmdEncoder setFragmentBytes:m_uniformData.data() length:m_uniformData.size() atIndex:0];
    
    const unsigned maxUniforms = m_shaderDesc->getNumUniforms();
    
    int uniformId = 1;
    for (unsigned idx = 0; idx < maxUniforms; ++idx)
    {
        GpuShaderDesc::UniformData data;
        m_shaderDesc->getUniform(idx, data);
        switch(data.m_type)
        {
            case UNIFORM_BOOL:
            case UNIFORM_DOUBLE:
            case UNIFORM_FLOAT3:
                // Part of uniform data buffer
            break;
                
            case UNIFORM_VECTOR_INT:
            {
                int size = data.m_vectorInt.m_getSize();
                const int dummyInt = 123456789;
                const int* v = size == 0 ? &dummyInt : data.m_vectorInt.m_getVector();
                size = size == 0 ? sizeof(int) : size * sizeof(int);
                [renderCmdEncoder setFragmentBytes:v length:size * sizeof(int) atIndex:uniformId++];
            }
            break;
                
            case UNIFORM_VECTOR_FLOAT:
            {
                int size = data.m_vectorFloat.m_getSize();
                const float dummyFloat = 123456789.0f;
                const float* v = size == 0 ? &dummyFloat : data.m_vectorFloat.m_getVector();
                size = size == 0 ? sizeof(float) : size * sizeof(float);
                [renderCmdEncoder setFragmentBytes:v length:size atIndex:uniformId++];
            }
            break;
                
            case UNIFORM_UNKNOWN:
                throw Exception("Unknown uniform type.");
        };
    }
}

bool MetalBuilder::buildPipelineStateObject(const std::string & clientShaderProgram)
{
    NSString* shaderSrc = [NSString stringWithUTF8String:clientShaderProgram.c_str()];
    
    NSError* error = nil;
    MTLCompileOptions* options = [MTLCompileOptions new];
    [options setLanguageVersion:MTLLanguageVersion2_0];
    [options setFastMathEnabled:NO];
    m_library = [[m_device newLibraryWithSource:shaderSrc options:options error:&error] autorelease];
    
    id<MTLFunction> vertexShader = [[m_library newFunctionWithName:@"ColorCorrectionVS"] autorelease];
    id<MTLFunction> pixelShader  = [[m_library newFunctionWithName:@"ColorCorrectionPS"] autorelease];
    
    MTLRenderPipelineDescriptor* renderPipelineDesc = [[MTLRenderPipelineDescriptor new] autorelease];
    [renderPipelineDesc setVertexFunction:vertexShader];
    [renderPipelineDesc setFragmentFunction:pixelShader];
    [renderPipelineDesc setVertexDescriptor:nil];
    [renderPipelineDesc setRasterizationEnabled:YES];
    [renderPipelineDesc setInputPrimitiveTopology:MTLPrimitiveTopologyClassTriangle];
    [[renderPipelineDesc colorAttachments][0] setPixelFormat:MTLPixelFormatRGBA32Float];
    m_PSO = [[m_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error] autorelease];
    
    if(error == nil)
        return true;
    
    return false;
}

void MetalBuilder::triggerProgrammaticCaptureScope()
{
    if (@available(macOS 10.15, *)) {
        MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
        MTLCaptureDescriptor* captureDescriptor = [[MTLCaptureDescriptor alloc] init];
        captureDescriptor.captureObject = m_device;

        NSError *error;
        if (![captureManager startCaptureWithDescriptor:captureDescriptor error:&error])
        {
            NSLog(@"Failed to start capture, error %@", error);
        }
    }
}

void MetalBuilder::stopProgrammaticCaptureScope()
{
    if (@available(macOS 10.15, *)) {
        MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
        [captureManager stopCapture];
    }
}

void MetalBuilder::applyColorCorrection(id<MTLTexture> inputTexture, id<MTLTexture> outputTexture, unsigned int outWidth, unsigned int outHeight)
{
    static bool captureThisFrame = false;
    
    if(captureThisFrame)
        triggerProgrammaticCaptureScope();
    
    id<MTLCommandBuffer> cmdBuffer = [m_cmdQueue commandBuffer];
    
    MTLRenderPassDescriptor* renderPassDesc = [[MTLRenderPassDescriptor new] autorelease];
    if (@available(macOS 10.15, *)) {
        [renderPassDesc setRenderTargetWidth:outWidth];
        [renderPassDesc setRenderTargetHeight:outHeight];
    } else {
        throw Exception("Metal Renderer Only Operates on MacOS 10.15 and above.");
    }
    [[renderPassDesc colorAttachments][0] setTexture:outputTexture];
    [[renderPassDesc colorAttachments][0] setLoadAction:MTLLoadActionClear];
    [[renderPassDesc colorAttachments][0] setStoreAction:MTLStoreActionStore];
    
    id<MTLRenderCommandEncoder> renderCmdEncoder = [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
    
    [renderCmdEncoder setRenderPipelineState:m_PSO];
    
    [renderCmdEncoder setFragmentTexture:inputTexture atIndex:0];
    
    setUniforms(renderCmdEncoder);
    
    const size_t max = m_textureIds.size();
    for (size_t idx=0; idx<max; ++idx)
    {
        const TextureId& data = m_textureIds[idx];
        [renderCmdEncoder setFragmentTexture:data.m_texture atIndex:(m_startIndex + idx)];
        [renderCmdEncoder setFragmentSamplerState:data.m_samplerState atIndex:(m_startIndex + idx)];
    }
    
    [renderCmdEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    
    [renderCmdEncoder endEncoding];
    [cmdBuffer commit];
    [cmdBuffer waitUntilCompleted];
    
    if(captureThisFrame)
        stopProgrammaticCaptureScope();
}

unsigned MetalBuilder::GetTextureMaxWidth()
{
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if ([device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v1] || [device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v2]) {
        return 8192;
    }
    return 16384;
}

} // namespace OCIO_NAMESPACE