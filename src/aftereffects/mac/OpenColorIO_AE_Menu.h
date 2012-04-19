
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//


#import <Cocoa/Cocoa.h>


@interface OpenColorIO_AE_Menu : NSView {
	NSArray *menu_items;
	NSInteger chosen_item;
}

- (id)init:(NSArray *)menuItems selectedItem:(NSInteger)selected;

- (void)showMenu;

- (IBAction)menuItemAction:(id)sender;

- (NSInteger)selectedItem;

@end
