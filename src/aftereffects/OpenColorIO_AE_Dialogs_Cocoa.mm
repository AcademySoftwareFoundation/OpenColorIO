

#include "OpenColorIO_AE_Dialogs.h"

#import <Cocoa/Cocoa.h>


bool OpenFile(char *path, int buf_len, ExtensionMap &extensions, void *hwnd)
{
	NSOpenPanel *op = [NSOpenPanel openPanel];
	 
	[op setTitle:@"OpenColorIO"];
	[op setMessage:@"Open a LUT"];
	
	
	NSMutableArray *extension_array = [[NSMutableArray alloc] init];
	
	for(ExtensionMap::const_iterator i = extensions.begin(); i != extensions.end(); i++)
	{
		[extension_array addObject:[NSString stringWithUTF8String:i->first.c_str()]];
	}
	
	
	int result = [op runModalForTypes:extension_array];
	
	
	[extension_array release];
	
	  
	if(result == NSOKButton)
	{
		return [[op filename] getCString:path maxLength:buf_len encoding:NSASCIIStringEncoding];
	}
	else
		return false;
}


void SetMickeyCursor()
{
	[[NSCursor pointingHandCursor] set];
}
