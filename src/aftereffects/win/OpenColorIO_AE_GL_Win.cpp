


#include "OpenColorIO_AE_GL.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

static HWND			g_win = NULL;
static HDC			g_hdc = NULL;
static HGLRC		g_context = NULL;
static GLuint		g_framebuffer;


static bool
HaveRequiredExtensions()
{
	const GLubyte *strVersion = glGetString(GL_VERSION);
	const GLubyte *strExt = glGetString(GL_EXTENSIONS);
	
	if(strVersion == NULL)
		return false;

	float gl_version;
	sscanf((char *)strVersion, "%f", &gl_version);
	
#define CheckExtension(N) glewIsExtensionSupported(N)

	return (gl_version >= 2.0 &&
			CheckExtension("GL_ARB_vertex_program") &&
			CheckExtension("GL_ARB_vertex_shader") &&
			CheckExtension("GL_ARB_texture_cube_map") &&
			CheckExtension("GL_ARB_fragment_shader") &&
			CheckExtension("GL_ARB_draw_buffers") &&
			CheckExtension("GL_ARB_framebuffer_object") &&
			CheckExtension("GL_EXT_framebuffer_object") );
}


void
GlobalSetup_GL()
{
	GLenum init = glewInit();

	if(init != GLEW_OK)
		return;


	WNDCLASSEX winClass; 
	MSG        uMsg;

	memset(&uMsg,0,sizeof(uMsg));

	winClass.lpszClassName = "OpenColorIO_AE_Win_Class";
	winClass.cbSize        = sizeof(WNDCLASSEX);
	winClass.style         = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc   = DefWindowProc;
	winClass.hInstance     = NULL;
	winClass.hIcon	       = NULL;
	winClass.hIconSm	   = NULL;
	winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winClass.lpszMenuName  = NULL;
	winClass.cbClsExtra    = 0;
	winClass.cbWndExtra    = 0;
	
	if( !( RegisterClassEx(&winClass) ) )
		return;

	g_win = CreateWindowEx( NULL, "OpenColorIO_AE_Win_Class", 
							 "OpenGL-using FBOs in AE",
								0, 0, 
								0, 50, 50,
								NULL, 
								NULL, 
								NULL,
								NULL	);

	if(g_win == NULL)
		return;

	GLuint PixelFormat;
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory( &pfd, sizeof( pfd ) );

	pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 128;
	pfd.cDepthBits = 32;
	
	
	g_hdc = GetDC(g_win);
	PixelFormat = ChoosePixelFormat(g_hdc, &pfd);
	SetPixelFormat(g_hdc, PixelFormat, &pfd);
	
	g_context = wglCreateContext(g_hdc);
	wglMakeCurrent(g_hdc, g_context);
	
	glFlush();
	

	if( g_context == NULL || !HaveRequiredExtensions() )
	{
		GlobalSetdown_GL();
		return;
	}
	
	glGenFramebuffersEXT(1, &g_framebuffer);
}


bool HaveOpenGL()
{
	return (g_context != NULL && g_win != NULL);
}


GLuint GetFrameBuffer()
{
	return g_framebuffer;
}


void
GlobalSetdown_GL()
{
	if(g_context)
	{
		wglDeleteContext(g_context);
		g_context = NULL;
		
		glDeleteFramebuffersEXT(1, &g_framebuffer);
	}
	
	if(g_win)
	{
		ReleaseDC(g_win, g_hdc);
		g_win = NULL;
		g_hdc = NULL;

		UnregisterClass("OpenColorIO_AE_Win_Class", NULL);
	}
}
