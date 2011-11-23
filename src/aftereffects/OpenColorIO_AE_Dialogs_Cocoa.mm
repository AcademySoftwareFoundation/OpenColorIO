

#include "OpenColorIO_AE_Dialogs.h"

#import <Cocoa/Cocoa.h>

#import "OpenColorIO_AE_Menu.h"


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


int PopUpMenu(MenuVec &menu_items, int selected_index)
{
	NSMutableArray *item_array = [[NSMutableArray alloc] init];
	
	for(MenuVec::const_iterator i = menu_items.begin(); i != menu_items.end(); i++)
	{
		[item_array addObject:[NSString stringWithUTF8String:i->c_str()]];
	}


	OpenColorIO_AE_Menu *menu = [[OpenColorIO_AE_Menu alloc] init:item_array selectedItem:selected_index];
	
	[menu showMenu];
	
	NSInteger item = [menu selectedItem];
	
	[menu release];
	[item_array release];
	
	return item;
}


void SetMickeyCursor()
{
	[[NSCursor pointingHandCursor] set];
}
