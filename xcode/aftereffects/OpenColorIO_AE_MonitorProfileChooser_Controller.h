
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//


#import <Cocoa/Cocoa.h>

@interface OpenColorIO_AE_MonitorProfileChooser_Controller : NSObject {
    IBOutlet NSPopUpButton *profileMenu;
	IBOutlet NSWindow *window;
	
	NSMutableArray	*name_array;
	NSMapTable		*profile_map;
}

- (IBAction)clickOK:(NSButton *)sender;
- (IBAction)clickCancel:(NSButton *)sender;

- (NSWindow *)getWindow;

- (BOOL)getMonitorProfile:(char *)path bufferSize:(int)buf_len;

@end
