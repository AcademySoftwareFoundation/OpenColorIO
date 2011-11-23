//
//  OCIO_AE_Menu.h
//  OpenColorIO_AE
//
//  Created by Brendan Bolles on 11/22/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface OpenColorIO_AE_Menu : NSView {
	NSArray *menu_items;
	NSInteger chosen_item;
}

// send in an array of NSStrings
- (id)init:(NSArray *)menuItems selectedItem:(NSInteger)selected;

- (void)showMenu;

- (IBAction)menuItemAction:(id)sender;

- (NSInteger)selectedItem;

@end
