/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "OpenColorIO_AE_GL.h"

#import <Cocoa/Cocoa.h>



static NSWindow     *g_win = nil;
static AGLContext   g_context = NULL;
static GLuint       g_framebuffer;


static bool HaveRequiredExtensions()
{
    const GLubyte *strVersion = glGetString(GL_VERSION);
    const GLubyte *strExt = glGetString(GL_EXTENSIONS);
    
    if(strVersion == NULL || strExt == NULL)
        return false;
        
    float gl_version;
    sscanf((char *)strVersion, "%f", &gl_version);
    
#define CheckExtension(N) gluCheckExtension((const GLubyte *)N, strExt)

    return (gl_version >= 2.0 &&
            CheckExtension("GL_ARB_color_buffer_float") &&
            CheckExtension("GL_ARB_texture_float") &&
            CheckExtension("GL_ARB_vertex_program") &&
            CheckExtension("GL_ARB_vertex_shader") &&
            CheckExtension("GL_ARB_texture_cube_map") &&
            CheckExtension("GL_ARB_fragment_shader") &&
            CheckExtension("GL_ARB_draw_buffers") &&
            CheckExtension("GL_ARB_framebuffer_object") );
}


void GlobalSetup_GL()
{
    GLint aAttribs[64];
    u_short nIndexS= -1;

    aAttribs[++nIndexS] = AGL_RGBA;
    aAttribs[++nIndexS] = AGL_DOUBLEBUFFER;
    aAttribs[++nIndexS] = AGL_COLOR_FLOAT;
    
    aAttribs[++nIndexS] = AGL_RED_SIZE;
    aAttribs[++nIndexS] = 32;
    aAttribs[++nIndexS] = AGL_GREEN_SIZE;
    aAttribs[++nIndexS] = 32;
    aAttribs[++nIndexS] = AGL_BLUE_SIZE;
    aAttribs[++nIndexS] = 32;
    aAttribs[++nIndexS] = AGL_ALPHA_SIZE;
    aAttribs[++nIndexS] = 32;
    
    aAttribs[++nIndexS] = AGL_NONE;
    
    
    AGLPixelFormat oPixelFormat = aglChoosePixelFormat(NULL, 0, aAttribs);
    
    if(oPixelFormat)
    {
        g_context = aglCreateContext(oPixelFormat, NULL);
        
        aglDestroyPixelFormat(oPixelFormat);
    }
    
    
    if(g_context == NULL)
        return;
    
    
    g_win = [[NSWindow alloc] initWithContentRect:NSZeroRect
                                styleMask:NSBorderlessWindowMask
                                backing:NSBackingStoreBuffered
                                defer:NO];
    
    
    aglSetDrawable(g_context, (AGLDrawable)[[g_win graphicsContext] graphicsPort]);
    
    glFlush();
    
    
    SetPluginContext();
    
    
    
    GLint units;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
    
    
    if( !HaveRequiredExtensions() || units < 2)
    {
        GlobalSetdown_GL();
        SetAEContext();
        return;
    }
    
    glGenFramebuffersEXT(1, &g_framebuffer);
    
    
    SetAEContext();
}


bool HaveOpenGL()
{
    return (g_context != NULL && g_win != nil);
}


static AGLContext g_ae_context;

void SetPluginContext()
{
    g_ae_context = aglGetCurrentContext();

    aglSetCurrentContext(g_context);
}


void SetAEContext()
{
    // g_ae_context might be NULL...so be it
    aglSetCurrentContext(g_ae_context);
}


GLuint GetFrameBuffer()
{
    return g_framebuffer;
}


void GlobalSetdown_GL()
{
    if(g_context)
    {
        aglDestroyContext(g_context);
        g_context = NULL;
        
        glDeleteFramebuffersEXT(1, &g_framebuffer);
    }
    
    if(g_win)
    {
        [g_win release];
        g_win = nil;
    }
}
