/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
    
    
    NSUInteger i;
    
    for(i=0; i < [menu_items count]; i++)
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
    
    
    id fakeMouseEvent=[NSEvent mouseEventWithType:NSLeftMouseDown
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
