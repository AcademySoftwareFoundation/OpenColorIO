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
    { kCVPixelFormatType_32BGRA,MTLPixelFormatBGRA8Unorm, GL_RGBA, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV },
    { kCVPixelFormatType_ARGB2101010LEPacked, MTLPixelFormatBGR10A2Unorm, GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV },
    { kCVPixelFormatType_32BGRA, MTLPixelFormatBGRA8Unorm_sRGB, GL_SRGB8_ALPHA8,   GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV },
    { kCVPixelFormatType_64RGBAHalf, MTLPixelFormatRGBA16Float, GL_RGBA, GL_RGBA, GL_HALF_FLOAT },
    { kCVPixelFormatType_128RGBAFloat, MTLPixelFormatRGBA32Float, GL_RGBA, GL_RGBA, GL_FLOAT},
};

const GLMetalTextureFormatInfo* textureFormatInfoFromMetalPixelFormat(MTLPixelFormat pixelFormat)
{
    int numInteropFromats = sizeof(GLMetalInteropFormatTable) / sizeof(GLMetalInteropFormatTable[0]);
    for(int i = 0; i < numInteropFromats; i++) {
        if(pixelFormat == GLMetalInteropFormatTable[i].mtlFormat)
        {
            return &GLMetalInteropFormatTable[i];
        }
    }
    return NULL;
}


MtlTexture::MtlTexture(id<MTLDevice> device, NSOpenGLContext* glContext, uint32_t width, uint32_t height, MTLPixelFormat pixelFormat, const float* image) : m_width(width), m_height(height)
{
    NSDictionary* cvBufferProperties = @{
        (__bridge NSString*)kCVPixelBufferOpenGLCompatibilityKey : @YES,
        (__bridge NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
    };

    m_device = device;
    m_openGLContext = glContext;
    
    m_formatInfo = textureFormatInfoFromMetalPixelFormat(pixelFormat);
    
    CVReturn cvret = CVPixelBufferCreate(kCFAllocatorDefault,
                            m_width, m_height,
                            m_formatInfo->cvPixelFormat,
                            (__bridge CFDictionaryRef)cvBufferProperties,
                            &m_CVPixelBuffer);
    
    assert(cvret == kCVReturnSuccess);
    
    m_CGLPixelFormat = m_openGLContext.pixelFormat.CGLPixelFormatObj;
    
    createGLTexture();
    createMetalTexture();
    
    if(image != nullptr)
        update(image);
}

void MtlTexture::createGLTexture()
{
    CVReturn cvret;
    // 1. Create an OpenGL CoreVideo texture cache from the pixel buffer.
    cvret  = CVOpenGLTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    m_openGLContext.CGLContextObj,
                    m_CGLPixelFormat,
                    nil,
                    &m_CVGLTextureCache);
    
    assert(cvret == kCVReturnSuccess && "Failed to create OpenGL Texture Cache");
    
    // 2. Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
    cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    m_CVGLTextureCache,
                    m_CVPixelBuffer,
                    nil,
                    &m_CVGLTexture);
    
    assert(cvret == kCVReturnSuccess && "Failed to create OpenGL Texture From Image");
    
    // 3. Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
    m_texID = CVOpenGLTextureGetName(m_CVGLTexture);
}

void MtlTexture::createMetalTexture()
{
    CVReturn cvret;
    // 1. Create a Metal Core Video texture cache from the pixel buffer.
    cvret = CVMetalTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    m_device,
                    nil,
                    &m_CVMTLTextureCache);

    assert(cvret == kCVReturnSuccess && "Failed to create Metal texture cache");
    
    // 2. Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
    
    cvret = CVMetalTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    m_CVMTLTextureCache,
                    m_CVPixelBuffer, nil,
                    m_formatInfo->mtlFormat,
                    m_width, m_height,
                    0,
                    &m_CVMTLTexture);
    
    assert(cvret == kCVReturnSuccess && "Failed to create CoreVideo Metal texture from image");
    
    // 3. Get a Metal texture using the CoreVideo Metal texture reference.
    m_metalTexture = CVMetalTextureGetTexture(m_CVMTLTexture);
    
    assert(m_metalTexture && "Failed to create Metal texture CoreVideo Metal Texture");
}

void MtlTexture::update(const float* image)
{
    std::vector<float> data = RGB_to_RGBA(image, 4 * m_width * m_height);
    
    [m_metalTexture replaceRegion:MTLRegionMake2D(0, 0, m_width, m_height) mipmapLevel:0 withBytes:image bytesPerRow:m_width * 4 * sizeof(float)];
}

MtlTexture::~MtlTexture()
{
    
}

}
