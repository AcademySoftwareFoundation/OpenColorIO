

#include "OpenColorIO_AE_Dialogs.h"

#import <Cocoa/Cocoa.h>

#import "OpenColorIO_AE_Menu.h"


bool OpenFile(char *path, int buf_len, ExtensionMap &extensions, void *hwnd)
{
	NSOpenPanel *op = [NSOpenPanel openPanel];
	 
	[op setTitle:@"OpenColorIO"];
	
	
	NSMutableArray *extension_array = [[NSMutableArray alloc] init];
	std::string message = "Formats: ";
	bool first_one = true;
	
	for(ExtensionMap::const_iterator i = extensions.begin(); i != extensions.end(); i++)
	{
		[extension_array addObject:[NSString stringWithUTF8String:i->first.c_str()]];
		
		if(first_one)
			first_one = false;
		else
			message += ", ";
			
		message += "." + i->first;
	}
	
	[op setMessage:[NSString stringWithUTF8String:message.c_str()]];
	
	
	NSInteger result = [op runModalForTypes:extension_array];
	
	
	[extension_array release];
	
	  
	if(result == NSOKButton)
	{
		return [[op filename] getCString:path maxLength:buf_len encoding:NSASCIIStringEncoding];
	}
	else
		return false;
}


bool SaveFile(char *path, int buf_len, ExtensionMap &extensions, void *hwnd)
{
	NSSavePanel *panel = [NSSavePanel savePanel];
	
	[panel setTitle:@"OpenColorIO"];
	
	
	NSMutableArray *extension_array = [[NSMutableArray alloc] init];
	std::string message = "Formats: ";
	bool first_one = true;
	
	for(ExtensionMap::const_iterator i = extensions.begin(); i != extensions.end(); i++)
	{
		[extension_array addObject:[NSString stringWithUTF8String:i->first.c_str()]];
		
		if(first_one)
			first_one = false;
		else
			message += ", ";
			
		message += i->second + " (." + i->first + ")";
	}
	
	[panel setAllowedFileTypes:extension_array];
	[panel setMessage:[NSString stringWithUTF8String:message.c_str()]];
	
	
	NSInteger result = [panel runModal];
	
	
	[extension_array release];
	
	
	if(result == NSOKButton)
	{
		return [[panel filename] getCString:path maxLength:buf_len encoding:NSASCIIStringEncoding];
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
