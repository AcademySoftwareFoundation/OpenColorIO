// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#import <Cocoa/Cocoa.h>

@interface OpenColorIO_AE_MonitorProfileChooser_Controller : NSWindowController
{
    NSMutableDictionary<NSString *, NSURL *>    *profile_dict;
}

@property (assign) IBOutlet NSPopUpButton *profileMenu;

- (IBAction)clickOK:(id)sender;
- (IBAction)clickCancel:(id)sender;

- (BOOL)getMonitorProfile:(char *)path bufferSize:(int)buf_len;

@end
