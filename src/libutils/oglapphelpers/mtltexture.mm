// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <vector>

#include "msl.h"
#include "mtltexture.h"


namespace OCIO_NAMESPACE
{

// Table of equivalent formats across CoreVideo, Metal, and OpenGL
static const GLMetalTextureFormatInfo GLMetalInteropFormatTable[] =
{
    // Core Video Pixel Format,Metal Pixel Format, GL internalformat, GL format, GL type
    { kCVPixelFormatType_32BGRA, MTLPixelFormatBGRA8Unorm,                 GL_RGBA,
      GL_BGRA_EXT,               GL_UNSIGNED_INT_8_8_8_8_REV },
    { kCVPixelFormatType_ARGB2101010LEPacked, MTLPixelFormatBGR10A2Unorm, GL_RGB10_A2,
      GL_BGRA,                   GL_UNSIGNED_INT_2_10_10_10_REV },
    { kCVPixelFormatType_32BGRA, MTLPixelFormatBGRA8Unorm_sRGB,           GL_SRGB8_ALPHA8,
      GL_BGRA,                   GL_UNSIGNED_INT_8_8_8_8_REV },
    { kCVPixelFormatType_64RGBAHalf, MTLPixelFormatRGBA16Float,           GL_RGBA,
      GL_RGBA,                   GL_HALF_FLOAT },
    { kCVPixelFormatType_128RGBAFloat, MTLPixelFormatRGBA32Float,         GL_RGBA,
      GL_RGBA,                   GL_FLOAT},
};

const GLMetalTextureFormatInfo* textureFormatInfoFromMetalPixelFormat(MTLPixelFormat pixelFormat)
{
    const int numInteropFormats =
        sizeof(GLMetalInteropFormatTable) / sizeof(GLMetalInteropFormatTable[0]);
    for(int i = 0; i < numInteropFormats; i++) {
        if(pixelFormat == GLMetalInteropFormatTable[i].mtlFormat)
        {
            return &GLMetalInteropFormatTable[i];
        }
    }
    return NULL;
}

MtlTexture::MtlTexture(id<MTLDevice> device, uint32_t width, uint32_t height, const float* image)
    : m_width(width), m_height(height)
{
    m_device = device;
    m_openGLContext = nullptr;
    MTLTextureDescriptor* texDescriptor = [MTLTextureDescriptor new];
 
    const int channelPerPix = 4;
    MTLPixelFormat pixelFormat = MTLPixelFormatRGBA32Float;
    
    [texDescriptor setWidth:width];
    [texDescriptor setHeight:height];
    [texDescriptor setDepth:1];
    [texDescriptor setStorageMode:MTLStorageModeManaged];
    [texDescriptor setPixelFormat:pixelFormat];
    [texDescriptor setMipmapLevelCount:1];
    m_metalTexture = [device newTextureWithDescriptor:texDescriptor];
    
    if(image)
    {
        [m_metalTexture replaceRegion:MTLRegionMake3D(0, 0, 0, width, height, 1)
                          mipmapLevel:0
                            withBytes:image
                          bytesPerRow:channelPerPix * width * sizeof(float)];
    }
    
    [texDescriptor release];
}

MtlTexture::MtlTexture(id<MTLDevice> device,
                       NSOpenGLContext* glContext,
                       uint32_t width, uint32_t height,
                       const float* image) : m_width(width), m_height(height)
{
    NSDictionary* cvBufferProperties = @{
        (__bridge NSString*)kCVPixelBufferOpenGLCompatibilityKey : @YES,
        (__bridge NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
    };

    m_device = device;
    m_openGLContext = glContext;
    
    m_formatInfo = textureFormatInfoFromMetalPixelFormat(MTLPixelFormatRGBA32Float);
    
    CVReturn cvret = CVPixelBufferCreate(kCFAllocatorDefault,
                            m_width, m_height,
                            m_formatInfo->cvPixelFormat,
                            (__bridge CFDictionaryRef)cvBufferProperties,
                            &m_CVPixelBuffer);
    
    if(cvret != kCVReturnSuccess)
    {
        throw Exception("Cannot create Pixel Buffer");
    }
    
    m_CGLPixelFormat = m_openGLContext.pixelFormat.CGLPixelFormatObj;
    
    createGLTexture();
    createMetalTexture();
    
    if(image != nullptr)
    {
        update(image);
    }
}

void MtlTexture::createGLTexture()
{
    CVReturn cvret;
    // Create an OpenGL CoreVideo texture cache from the pixel buffer.
    cvret  = CVOpenGLTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    m_openGLContext.CGLContextObj,
                    m_CGLPixelFormat,
                    nil,
                    &m_CVGLTextureCache);
    
    if(cvret != kCVReturnSuccess)
    {
        throw Exception("Failed to create OpenGL Texture Cache");
    }
    
    // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
    cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    m_CVGLTextureCache,
                    m_CVPixelBuffer,
                    nil,
                    &m_CVGLTexture);
    
    if(cvret != kCVReturnSuccess)
    {
        throw Exception("Failed to create OpenGL Texture From Image");
    }
    
    // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
    m_texID = CVOpenGLTextureGetName(m_CVGLTexture);
}

void MtlTexture::createMetalTexture()
{
    CVReturn cvret;
    // Create a Metal Core Video texture cache from the pixel buffer.
    cvret = CVMetalTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    m_device,
                    nil,
                    &m_CVMTLTextureCache);

    if(cvret != kCVReturnSuccess)
    {
        throw Exception("Failed to create Metal texture cache");
    }
    
    // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache
    cvret = CVMetalTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    m_CVMTLTextureCache,
                    m_CVPixelBuffer, nil,
                    m_formatInfo->mtlFormat,
                    m_width, m_height,
                    0,
                    &m_CVMTLTexture);
    
    if(cvret != kCVReturnSuccess)
    {
        throw Exception("Failed to create CoreVideo Metal texture from image");
    }
    
    // Get a Metal texture using the CoreVideo Metal texture reference.
    m_metalTexture = CVMetalTextureGetTexture(m_CVMTLTexture);
    
    if(!m_metalTexture)
    {
        throw Exception("Failed to create Metal texture CoreVideo Metal Texture");
    }
}

void MtlTexture::update(const float* image)
{
    [m_metalTexture replaceRegion:MTLRegionMake2D(0, 0, m_width, m_height)
                      mipmapLevel:0
                        withBytes:image
                      bytesPerRow:m_width * 4 * sizeof(float)];
}

std::vector<float> MtlTexture::readTexture() const
{
    CVPixelBufferLockBaseAddress(m_CVPixelBuffer, 0);
    size_t dataSize = CVPixelBufferGetDataSize(m_CVPixelBuffer);
    float* data = (float*)CVPixelBufferGetBaseAddress(m_CVPixelBuffer);//(m_CVPixelBuffer, 0);
    std::vector<float> img(4 * m_width * m_height);
    if((img.size()*sizeof(float)) < dataSize)
    {
        throw Exception("CPU-side vector is not large enough to read the texture back.");
    }
    memcpy(img.data(), data, dataSize);
    CVPixelBufferUnlockBaseAddress(m_CVPixelBuffer, 0);
    return img;
}

MtlTexture::~MtlTexture()
{
    if(m_openGLContext)
        [m_openGLContext release];
}

}
