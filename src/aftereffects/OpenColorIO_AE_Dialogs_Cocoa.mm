
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//

#include "OpenColorIO_AE_Dialogs.h"

#import "OpenColorIO_AE_MonitorProfileChooser_Controller.h"

#import "OpenColorIO_AE_Menu.h"


bool OpenFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	 
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
			
		message += "." + i->first;
	}
	
	[panel setMessage:[NSString stringWithUTF8String:message.c_str()]];
	
	
	NSInteger result = [panel runModalForTypes:extension_array];
	
	
	[extension_array release];
	
	  
	if(result == NSOKButton)
	{
		return [[panel filename] getCString:path maxLength:buf_len encoding:NSASCIIStringEncoding];
	}
	else
		return false;
}


bool SaveFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd)
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


bool GetMonitorProfile(char *path, int buf_len, const void *hwnd)
{
	bool hit_ok = false;
	
	Class ui_controller_class = [[NSBundle bundleWithIdentifier:@"org.OpenColorIO.AfterEffects"]
									classNamed:@"OpenColorIO_AE_MonitorProfileChooser_Controller"];
	
	if(ui_controller_class)
	{
		OpenColorIO_AE_MonitorProfileChooser_Controller *ui_controller = [[ui_controller_class alloc] init];
		
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger result = [NSApp runModalForWindow:my_window];
				
				if(result == NSRunStoppedResponse)
				{
					[ui_controller getMonitorProfile:path bufferSize:buf_len];
					
					hit_ok = true;
				}
				
				[my_window release];
			}
			
			[ui_controller release];
		}
	}
	
	return hit_ok;
}


int PopUpMenu(const MenuVec &menu_items, int selected_index, const void *hwnd)
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


void ErrorMessage(const char *message, const void *hwnd)
{
	NSAlert *alert = [[NSAlert alloc] init];
	
	[alert setMessageText:[NSString stringWithUTF8String:message]];
										
	[alert runModal];
	
	[alert release];
}


void SetMickeyCursor()
{
	[[NSCursor pointingHandCursor] set];
}
