// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef _OPENCOLORIO_AE_GL_H_
#define _OPENCOLORIO_AE_GL_H_

#include "AEConfig.h"

#ifdef AE_OS_WIN
    #include <GL/glew.h>
    #include <GL/wglew.h>
#elif defined(AE_OS_MAC)
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
    #include <OpenGL/glext.h>
    #include <AGL/agl.h>
#endif


void GlobalSetup_GL();

bool HaveOpenGL();

// must surround plug-in OpenGL calls with these functions so that AE
// doesn't know we're borrowing the OpenGL renderer
void SetPluginContext();
void SetAEContext();

GLuint GetFrameBuffer();

void GlobalSetdown_GL();


#endif // _OPENCOLORIO_AE_GL_H_