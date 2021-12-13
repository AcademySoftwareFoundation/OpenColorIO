// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_MTLTEXTURE_H
#define INCLUDED_OCIO_MTLTEXTURE_H

#include <vector>
#include <OpenGL/gl.h>

#import  <AppKit/AppKit.h>
#import  <Metal/Metal.h>
#import  <CoreVideo/CVPixelBuffer.h>
#import  <CoreVideo/CVOpenGLTextureCache.h>
#import  <CoreVideo/CVMetalTextureCache.h>

#include <OpenColorIO/OpenColorIO.h>

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
    uint64_t    getGLHandle()    const
    {
        if(!m_openGLContext)
        {
            throw Exception("There is no valid OpenGL Context for this texture");
        }
        return m_texID;
    }
    
    ~MtlTexture();
    
    MtlTexture(id<MTLDevice> device, uint32_t width, uint32_t height, const float* image);
    MtlTexture(id<MTLDevice> device, NSOpenGLContext* glContext, uint32_t width, uint32_t height, const float* image);
    void update(const float* image);
    id<MTLTexture> getMetalTextureHandle() const { return m_metalTexture; }
    std::vector<float> readTexture() const;
    
private:
    void createGLTexture();
    void createMetalTexture();
    
    id<MTLDevice>                   m_device;
    NSOpenGLContext*                m_openGLContext;
    
    int                             m_width;
    int                             m_height;
    
    unsigned int                    m_texID;
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

