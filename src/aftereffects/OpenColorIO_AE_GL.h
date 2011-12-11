

#ifndef _OPENCOLORIO_AE_GL_H_
#define _OPENCOLORIO_AE_GL_H_

#include "AEConfig.h"

#ifdef AE_OS_WIN
	//#include <GL/glew.h>
typedef unsigned int GLuint;
#elif defined(AE_OS_MAC)
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
	#include <OpenGL/glext.h>
	#include <AGL/agl.h>
#endif


void GlobalSetup_GL();

bool HaveOpenGL();

GLuint GetFrameBuffer();

void GlobalSetdown_GL();


#endif // _OPENCOLORIO_AE_GL_H_