// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#import "OpenColorIO_AE_Menu.h"


@implementation OpenColorIO_AE_Menu

- (id)init:(NSArray *)menuItems selectedItem:(NSInteger)selected
{
    self = [super init];
    
    menu_items = menuItems;
    chosen_item = selected;
    
    return self;
}

- (void)showMenu
{

    // Doing some pretty weird stuff here.
    // I need to bring up a contextual menu without AE giving me an NSView.
    // To get the screen position, I use global NSApp methods.
    // And I need to make an NSView to give to popUpContextMenu:
    // so I can catch the selectors it sends, so I made this an NSView subclass.
    
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Pop-Up"];
    
    [menu setAutoenablesItems:FALSE];
    
    
    for(NSUInteger i=0; i < [menu_items count]; i++)
    {
        if( [[menu_items objectAtIndex:i] isEqualToString:@"(-"] )
        {
            [menu addItem:[NSMenuItem separatorItem]];
        }
        else
        {
            NSMenuItem *item = [menu addItemWithTitle:[menu_items objectAtIndex:i]
                                    action:@selector(menuItemAction:)
                                    keyEquivalent:@""];
            
            [item setTag:i];
            
            if(i == chosen_item)
                [item setState:NSOnState];
                
            if([[menu_items objectAtIndex:i] isEqualToString:@"$OCIO"] && NULL == getenv("OCIO"))
            {
                [item setEnabled:FALSE];
                [item setState:NSOffState];
            }
            else if([[menu_items objectAtIndex:i] isEqualToString:@"(nada)"])
            {
                [item setTitle:@"No configs in /Library/Application Support/OpenColorIO/"];
                [item setEnabled:FALSE];
            }
        }
    }
    
    
    id fakeMouseEvent = [NSEvent mouseEventWithType:NSLeftMouseDown
                        location: [[NSApp keyWindow] convertScreenToBase:[NSEvent mouseLocation]]
                        modifierFlags:0 
                        timestamp:0
                        windowNumber: [[NSApp keyWindow] windowNumber]
                        context:nil
                        eventNumber:0 
                        clickCount:1 
                        pressure:0];
    
    [NSMenu popUpContextMenu:menu withEvent:fakeMouseEvent forView:self];

    [menu release];
}

- (IBAction)menuItemAction:(id)sender
{
    NSMenuItem *item = (NSMenuItem *)sender;
    
    chosen_item = [item tag];
}

- (NSInteger)selectedItem
{
    return chosen_item;
}



- (id)initWithTextMenu:(NSMenu *)menu
{
    self = [super init];
    
    text_menu = menu;
    chosen_menu_item = nil;
    
    return self;
}

- (void)showTextMenu
{
    id fakeMouseEvent=[NSEvent mouseEventWithType:NSLeftMouseDown
                        location: [[NSApp keyWindow] convertScreenToBase:[NSEvent mouseLocation]]
                        modifierFlags:0 
                        timestamp:0
                        windowNumber: [[NSApp keyWindow] windowNumber]
                        context:nil
                        eventNumber:0 
                        clickCount:1 
                        pressure:0];
    
    [NSMenu popUpContextMenu:text_menu withEvent:fakeMouseEvent forView:self];
}

- (IBAction)textMenuItemAction:(id)sender
{
    NSMenuItem *item = (NSMenuItem *)sender;
    
    chosen_menu_item = item;
}

- (NSMenuItem *)selectedTextMenuItem
{
    return chosen_menu_item;
}

@end
