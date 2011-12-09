


#include "OpenColorIO_AE_GL.h"

#import <Cocoa/Cocoa.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include <AGL/agl.h>


static NSWindow		*g_win = nil;
static AGLContext	g_context = NULL;


PF_Err
GlobalSetup_GL ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err err = PF_Err_NONE;

	//NSRect rect = NSMakeRect(0, 0, 50, 50);
	
	g_win = [[NSWindow alloc] initWithContentRect:NSZeroRect //rect
								styleMask:NSBorderlessWindowMask
								backing:NSBackingStoreBuffered
								defer:NO];
	
	
	GLint aAttribs[64];
	u_short nIndexS= -1;

	// NO color index support
	aAttribs[++nIndexS]= AGL_RGBA;
	aAttribs[++nIndexS]= AGL_DOUBLEBUFFER;
	
	// color
	aAttribs[++nIndexS] = AGL_RED_SIZE;
	aAttribs[++nIndexS] = 8;
	aAttribs[++nIndexS] = AGL_GREEN_SIZE;
	aAttribs[++nIndexS] = 8;
	aAttribs[++nIndexS] = AGL_BLUE_SIZE;
	aAttribs[++nIndexS] = 8;
	aAttribs[++nIndexS] = AGL_ALPHA_SIZE;
	aAttribs[++nIndexS] = 8;
	
	aAttribs[++nIndexS] = AGL_NONE;

	// get an appropriate pixel format
	AGLPixelFormat oPixelFormat = aglChoosePixelFormat(NULL, 0, aAttribs);

	if(oPixelFormat)
	{
		g_context = aglCreateContext(oPixelFormat, NULL);
		
		aglDestroyPixelFormat(oPixelFormat);
	}
	
	
	if(oPixelFormat == NULL || g_context == NULL)
	{
		[g_win release];
		g_win = nil;
		return err;
	}
	
	
	aglSetDrawable(g_context, (AGLDrawable)[[g_win graphicsContext] graphicsPort]);


	glFlush();
	aglSetCurrentContext(g_context);


	return err;
}


PF_Err
GlobalSetdown_GL ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err err = PF_Err_NONE;
	
	if(g_context)
		aglDestroyContext(g_context);
	
	if(g_win)
		[g_win release];
	
	return err;
}
