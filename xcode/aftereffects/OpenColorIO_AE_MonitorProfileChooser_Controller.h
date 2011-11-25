//
//  OpenColorIO_AE_MonitorProfileChooser_Controller.h
//
//  Created by Brendan Bolles on 11/25/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface OpenColorIO_AE_MonitorProfileChooser_Controller : NSObject {
    IBOutlet NSPopUpButton *profileMenu;
	IBOutlet NSWindow *window;
	
	NSMutableArray *path_array;
	NSMutableArray *name_array;
}

- (id)init;

- (IBAction)clickOK:(NSButton *)sender;
- (IBAction)clickCancel:(NSButton *)sender;

- (NSWindow *)getWindow;

- (BOOL)getMonitorProfile:(char *)path bufferSize:(int)buf_len;

@end
