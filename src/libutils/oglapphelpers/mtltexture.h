// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_MTLTEXTURE_H
#define INCLUDED_OCIO_MTLTEXTURE_H

#include <OpenGL/gl.h>

#import  <AppKit/AppKit.h>
#import  <Metal/Metal.h>
#import  <CoreVideo/CVPixelBuffer.h>
#import  <CoreVideo/CVOpenGLTextureCache.h>
#import  <CoreVideo/CVMetalTextureCache.h>

namespace OCIO_NAMESPACE
{

typedef struct {
    int                 cvPixelFormat;
    MTLPixelFormat      mtlFormat;
    GLuint              glInternalFormat;
    GLuint              glFormat;
    GLuint              glType;
} GLMetalTextureFormatInfo;

class MtlTexture
{
public:
    MtlTexture() = delete;
    MtlTexture(const MtlTexture&) = delete;
    MtlTexture & operator=(const MtlTexture&) = delete;
    
    int         getWidth()       const { return m_width;  }
    int         getHeight()      const { return m_height; }
    uint64_t    getGLHandle()    const { return m_texID; }
    
    ~MtlTexture();
    
    MtlTexture(id<MTLDevice> device, NSOpenGLContext* glContext, uint32_t width, uint32_t height, MTLPixelFormat pixelFormat, const float* image);
    void update(const float* image);
    
    void flushGL()
    {
        CVOpenGLTextureCacheFlush(m_CVGLTextureCache, 0);
    }
    
    id<MTLTexture> getMetalTextureHandle() const { return m_metalTexture; }
    GLuint getOpenGLTextureName() { return (m_texID = CVOpenGLTextureGetName(m_CVGLTexture)); }

private:
    void createGLTexture();
    void createMetalTexture();
    
    id<MTLDevice>                   m_device;
    NSOpenGLContext*                m_openGLContext;
    
    int m_width;
    int m_height;
    
    unsigned int m_texID;
    
    id<MTLTexture>                  m_metalTexture;
    
    const GLMetalTextureFormatInfo* m_formatInfo;
    CVPixelBufferRef                m_CVPixelBuffer;
    CVMetalTextureRef               m_CVMTLTexture;

    CVOpenGLTextureCacheRef         m_CVGLTextureCache;
    CVOpenGLTextureRef              m_CVGLTexture;
    CGLPixelFormatObj               m_CGLPixelFormat;

    // Metal
    CVMetalTextureCacheRef          m_CVMTLTextureCache;
};


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MTLTEXTURE_H

