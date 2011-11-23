//
//  OCIO_AE_Menu.m
//  OpenColorIO_AE
//
//  Created by Brendan Bolles on 11/22/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import "OpenColorIO_AE_Menu.h"


@implementation OpenColorIO_AE_Menu

- (id)init:(NSArray *)menuItems selectedItem:(NSInteger)selected {
	self = [super init];

	menu_items = menuItems;
	chosen_item = selected;
	
	return self;
}

- (void)showMenu {

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
		NSMenuItem *item = [menu addItemWithTitle:[menu_items objectAtIndex:i]
								action:@selector(menuItemAction:)
								keyEquivalent:@""];
		
		[item setTag:i];
		
		if(i == chosen_item)
			[item setState:NSOnState];
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

- (IBAction)menuItemAction:(id)sender {
	NSMenuItem *item = (NSMenuItem *)sender;
	
	chosen_item = [item tag];
}

- (NSInteger)selectedItem {
	return chosen_item;
}

@end
