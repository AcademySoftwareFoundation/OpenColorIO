// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#import <Cocoa/Cocoa.h>


@interface OpenColorIO_AE_Menu : NSView
{
    NSArray *menu_items;
    NSInteger chosen_item;
    
    NSMenu *text_menu;
    NSMenuItem *chosen_menu_item;
}

// index-based menu
- (id)init:(NSArray *)menuItems selectedItem:(NSInteger)selected;

- (void)showMenu;

- (IBAction)menuItemAction:(id)sender;

- (NSInteger)selectedItem;

// text-based menu
- (id)initWithTextMenu:(NSMenu *)menu;

- (void)showTextMenu;

- (IBAction)textMenuItemAction:(id)sender;

- (NSMenuItem *)selectedTextMenuItem;

@end
