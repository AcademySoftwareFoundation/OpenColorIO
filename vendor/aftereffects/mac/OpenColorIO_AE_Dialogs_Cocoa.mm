// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
        return [[panel filename] getCString:path
                                maxLength:buf_len
                                encoding:NSASCIIStringEncoding];
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
        return [[panel filename] getCString:path
                                    maxLength:buf_len
                                    encoding:NSASCIIStringEncoding];
    }
    else
        return false;
}


bool GetMonitorProfile(char *path, int buf_len, const void *hwnd)
{
    bool hit_ok = false;
    
    OpenColorIO_AE_MonitorProfileChooser_Controller *ui_controller = [[OpenColorIO_AE_MonitorProfileChooser_Controller alloc] init];
    
    if(ui_controller)
    {
        NSWindow *my_window = [ui_controller window];
        
        if(my_window)
        {
            NSInteger result = [NSApp runModalForWindow:my_window];
            
            if(result == NSRunStoppedResponse)
            {
                [ui_controller getMonitorProfile:path bufferSize:buf_len];
                
                hit_ok = true;
            }
        
            [ui_controller close];
        }

        [ui_controller release];
    }
    
    return hit_ok;
}


void GetStdConfigs(ConfigVec &configs)
{
    NSString *ocioDir = @"/Library/Application Support/OpenColorIO";

    NSFileManager *man = [NSFileManager defaultManager];

    NSDirectoryEnumerator *enumerator = [man enumeratorAtPath:ocioDir];
    
    NSMutableOrderedSet<NSString *> *configSet = [NSMutableOrderedSet orderedSet];
    
    for(NSString *file in enumerator)
    {
        NSString *configPath = [[ocioDir stringByAppendingPathComponent:file] stringByAppendingPathComponent:@"config.ocio"];
                
        [enumerator skipDescendents];
    
        if([man fileExistsAtPath:configPath])
        {
            [configSet addObject:file];
        }
    }
    
    [configSet sortUsingComparator:
                    ^(NSString *str1, NSString *str2)
                    {
                        return [str1 compare:str2];
                    }];
    
    [configSet enumerateObjectsUsingBlock:
                    ^(NSString *str, NSUInteger idx, BOOL *stop)
                    {
                        configs.push_back( [str UTF8String] );
                    }];
}


std::string GetStdConfigPath(const std::string &name)
{
    const char *ocio_dir = "/Library/Application Support/OpenColorIO/";
    
    std::string config_path = ocio_dir + name + "/config.ocio";
    
    if( [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:config_path.c_str()]] )
    {
        return config_path;
    }
    else
    {
        return "";
    }
}


int PopUpMenu(const MenuVec &menu_items, int selected_index, const void *hwnd)
{
    NSMutableArray *item_array = [NSMutableArray array];
    
    for(MenuVec::const_iterator i = menu_items.begin(); i != menu_items.end(); i++)
    {
        [item_array addObject:[NSString stringWithUTF8String:i->c_str()]];
    }


    OpenColorIO_AE_Menu *menu = [[OpenColorIO_AE_Menu alloc] init:item_array
                                    selectedItem:selected_index];
    
    [menu showMenu];
    
    NSInteger item = [menu selectedItem];
    
    [menu release];
    
    return item;
}


bool ColorSpacePopUpMenu(OCIO::ConstConfigRcPtr config, std::string &colorSpace, bool selectRoles, const void *hwnd)
{
    NSMenu *menu = [[[NSMenu alloc] initWithTitle:@"ColorSpace Pop-Up"] autorelease];
    
    [menu setAutoenablesItems:NO];
    
    
    NSMutableDictionary<NSString *, NSMutableOrderedSet<NSString *> *> *categoriesDict = [NSMutableDictionary dictionary];
    NSMutableDictionary<NSString *, NSMutableOrderedSet<NSString *> *> *encodingsDict = [NSMutableDictionary dictionary];
    
    const int numColorSpaces = config->getNumColorSpaces();
    
    for(int i=0; i < numColorSpaces; ++i)
    {
        const char *colorSpaceName = config->getColorSpaceNameByIndex(i);
        
        OCIO::ConstColorSpaceRcPtr colorSpacePtr = config->getColorSpace(colorSpaceName);
        
        const char *family = colorSpacePtr->getFamily();
        
        
        NSString *colorSpacePath = nil;
        
        if(family == NULL || family == std::string(""))
        {
            colorSpacePath = [NSString stringWithUTF8String:colorSpaceName];
        }
        else
        {
            colorSpacePath = [NSString stringWithFormat:@"%s/%s", family, colorSpaceName];
        }
        
        
        assert(config->getFamilySeparator() == '/');
        
        NSArray *pathComponents = [colorSpacePath pathComponents];
        
        NSMenu *currentMenu = menu;
        
        for(int j=0; j < [pathComponents count]; j++)
        {
            NSString *componentName = [pathComponents objectAtIndex:j];
            
            if(j == ([pathComponents count] - 1))
            {
                NSMenuItem *newItem = [currentMenu addItemWithTitle:componentName action:@selector(textMenuItemAction:) keyEquivalent:@""];
                
                if(colorSpace == [componentName UTF8String])
                {
                    [newItem setState:NSOnState];
                }
            }
            else
            {
                NSMenuItem *componentItem = [currentMenu itemWithTitle:componentName];
                
                if(componentItem == nil)
                {
                    componentItem = [currentMenu addItemWithTitle:componentName action:NULL keyEquivalent:@""];
                    
                    NSMenu *subMenu = [[[NSMenu alloc] initWithTitle:componentName] autorelease];
                    
                    [subMenu setAutoenablesItems:NO];
                    
                    [componentItem setSubmenu:subMenu];
                }
                
                currentMenu = [componentItem submenu];
            }
        }
        
        for(int j=0; j < colorSpacePtr->getNumCategories(); j++)
        {
            const char *categoryName = colorSpacePtr->getCategory(j);
                
            NSString *category = [NSString stringWithUTF8String:categoryName];
                
            NSMutableOrderedSet<NSString *> *categorySet = [categoriesDict objectForKey:category];
                
            if(categorySet == nil)
            {
                categorySet = [NSMutableOrderedSet orderedSet];
                    
                [categoriesDict setObject:categorySet forKey:category];
            }
                
            [categorySet addObject:[NSString stringWithUTF8String:colorSpaceName]];
        }
        
        const char *encoding = colorSpacePtr->getEncoding();
        
        if(encoding != NULL && encoding != std::string(""))
        {
            NSString *encodingName = [NSString stringWithUTF8String:encoding];
            
            NSMutableOrderedSet<NSString *> *encodingSet = [encodingsDict objectForKey:encodingName];
            
            if(encodingSet == nil)
            {
                encodingSet = [NSMutableOrderedSet orderedSet];
                
                [encodingsDict setObject:encodingSet forKey:encodingName];
            }
            
            [encodingSet addObject:[NSString stringWithUTF8String:colorSpaceName]];
        }
    }
    
    if([encodingsDict count] > 0 || [categoriesDict count] > 0 || config->getNumRoles() > 0)
        [menu insertItem:[NSMenuItem separatorItem] atIndex:0];
    
    if([encodingsDict count] > 0)
    {
        NSMenuItem *encodingsItem = [menu insertItemWithTitle:@"Encodings" action:NULL keyEquivalent:@"" atIndex:0];
        
        NSMenu *encodingsMenu = [[[NSMenu alloc] initWithTitle:@"Encodings"] autorelease];
        
        [encodingsMenu setAutoenablesItems:NO];
        
        [encodingsItem setSubmenu:encodingsMenu];
        
        NSArray<NSString *> *encodingsUnsorted = [encodingsDict allKeys];
        
        NSMutableArray<NSString *> *encodings = [NSMutableArray arrayWithArray:encodingsUnsorted];
        
        [encodings sortUsingComparator:
                        ^(NSString *str1, NSString *str2)
                        {
                            return [str1 compare:str2];
                        }];

        for(int i=0; i < [encodings count]; i++)
        {
            NSString *encoding = [encodings objectAtIndex:i];
            
            NSMenuItem *encodingItem = [encodingsMenu addItemWithTitle:encoding action:NULL keyEquivalent:@""];
            
            NSMenu *encodingMenu = [[[NSMenu alloc] initWithTitle:encoding] autorelease];
            
            [encodingMenu setAutoenablesItems:NO];
            
            [encodingItem setSubmenu:encodingMenu];
            
            NSMutableOrderedSet<NSString *> *encodingSet = [encodingsDict objectForKey:encoding];
            
            [encodingSet sortUsingComparator:
                            ^(NSString *str1, NSString *str2)
                            {
                                return [str1 compare:str2];
                            }];
            
            for(int j=0; j < [encodingSet count]; j++)
            {
                NSString *colorSpaceName = [encodingSet objectAtIndex:j];
                
                NSMenuItem *encodingColorSpaceItem = [encodingMenu addItemWithTitle:colorSpaceName action:@selector(textMenuItemAction:) keyEquivalent:@""];
                
                if(colorSpace == [colorSpaceName UTF8String])
                {
                    [encodingColorSpaceItem setState:NSOnState];
                }
            }
        }
    }
    
    if([categoriesDict count] > 0)
    {
        NSMenuItem *categoriesItem = [menu insertItemWithTitle:@"Categories" action:NULL keyEquivalent:@"" atIndex:0];
        
        NSMenu *categoriesMenu = [[[NSMenu alloc] initWithTitle:@"Categories"] autorelease];
        
        [categoriesMenu setAutoenablesItems:NO];
        
        [categoriesItem setSubmenu:categoriesMenu];
        
        NSArray<NSString *> *categoriesUnsorted = [categoriesDict allKeys];
        
        NSMutableArray<NSString *> *categories = [NSMutableArray arrayWithArray:categoriesUnsorted];
        
        [categories sortUsingComparator:
                        ^(NSString *str1, NSString *str2)
                        {
                            return [str1 compare:str2];
                        }];

        for(int i=0; i < [categories count]; i++)
        {
            NSString *category = [categories objectAtIndex:i];
            
            NSMenuItem *categoryItem = [categoriesMenu addItemWithTitle:category action:NULL keyEquivalent:@""];
            
            NSMenu *categoryMenu = [[[NSMenu alloc] initWithTitle:category] autorelease];
            
            [categoryMenu setAutoenablesItems:NO];
            
            [categoryItem setSubmenu:categoryMenu];
            
            NSMutableOrderedSet<NSString *> *categorySet = [categoriesDict objectForKey:category];
            
            [categorySet sortUsingComparator:
                            ^(NSString *str1, NSString *str2)
                            {
                                return [str1 compare:str2];
                            }];
            
            for(int j=0; j < [categorySet count]; j++)
            {
                NSString *colorSpaceName = [categorySet objectAtIndex:j];
                
                NSMenuItem *categoryColorSpaceItem = [categoryMenu addItemWithTitle:colorSpaceName action:@selector(textMenuItemAction:) keyEquivalent:@""];
                
                if(colorSpace == [colorSpaceName UTF8String])
                {
                    [categoryColorSpaceItem setState:NSOnState];
                }
            }
        }
    }
    
    if(config->getNumRoles() > 0)
    {
        NSMenuItem *rolesItem = [menu insertItemWithTitle:@"Roles" action:NULL keyEquivalent:@"" atIndex:0];
        
        NSMenu *rolesMenu = [[[NSMenu alloc] initWithTitle:@"Roles"] autorelease];
        
        [rolesMenu setAutoenablesItems:NO];
        
        [rolesItem setSubmenu:rolesMenu];
        
        for(int i=0; i < config->getNumRoles(); i++)
        {
            NSString *roleName = [NSString stringWithUTF8String:config->getRoleName(i)];
            
            OCIO::ConstColorSpaceRcPtr colorSpacePtr = config->getColorSpace([roleName UTF8String]);
            
            NSString *colorSpaceName = [NSString stringWithUTF8String:colorSpacePtr->getName()];
            
            SEL selector = (selectRoles ? @selector(textMenuItemAction:) : NULL);
            
            NSMenuItem *roleItem = [rolesMenu addItemWithTitle:roleName action:selector keyEquivalent:@""];
            
            if(colorSpace == [roleName UTF8String])
            {
                [roleItem setState:NSOnState];
            }
            
            NSMenu *roleMenu = [[[NSMenu alloc] initWithTitle:roleName] autorelease];
            
            [roleMenu setAutoenablesItems:NO];
            
            [roleItem setSubmenu:roleMenu];
            
            NSMenuItem *roleColorSpaceItem = [roleMenu addItemWithTitle:colorSpaceName action:@selector(textMenuItemAction:) keyEquivalent:@""];
            
            if(colorSpace == [colorSpaceName UTF8String])
            {
                [roleColorSpaceItem setState:NSOnState];
            }
        }
    }
    
        
    
    OpenColorIO_AE_Menu *ocio_menu = [[OpenColorIO_AE_Menu alloc] initWithTextMenu:menu];
    
    [ocio_menu showTextMenu];
    
    
    NSMenuItem *item = [ocio_menu selectedTextMenuItem];
    
    
    bool selected = false;
    
    if(item != nil)
    {
        colorSpace = [[item title] UTF8String];
        
        selected = true;
    }
    
    
    [ocio_menu release];
    
    
    return selected;
}


void ErrorMessage(const char *message, const void *hwnd)
{
    NSAlert *alert = [[NSAlert alloc] init];
    
    [alert setMessageText:[NSString stringWithUTF8String:message]];
                                        
    [alert runModal];
    
    [alert release];
}

