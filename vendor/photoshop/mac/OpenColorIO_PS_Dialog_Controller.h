// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#import <Cocoa/Cocoa.h>

typedef enum ControllerSource
{
    CSOURCE_ENVIRONMENT,
    CSOURCE_STANDARD,
    CSOURCE_CUSTOM
};

typedef enum ControllerAction
{
    CACTION_LUT,
    CACTION_CONVERT,
    CACTION_DISPLAY
};

typedef enum ControllerInterp
{
    CINTERP_NEAREST,
    CINTERP_LINEAR,
    CINTERP_TETRAHEDRAL,
    CINTERP_CUBIC,
    CINTERP_BEST
};
 

@class OpenColorIO_AE_MonitorProfileChooser_Controller;

@interface OpenColorIO_PS_Dialog_Controller : NSObject
{
    IBOutlet NSPopUpButton *configurationMenu;
    IBOutlet NSMatrix *actionRadios;
    IBOutlet NSTextField *label1;
    IBOutlet NSTextField *label2;
    IBOutlet NSTextField *label3;
    IBOutlet NSPopUpButton *menu1;
    IBOutlet NSPopUpButton *menu2;
    IBOutlet NSPopUpButton *menu3;
    IBOutlet NSButton *invertCheck;
    IBOutlet NSButton *inputSpaceButton;
    IBOutlet NSButton *outputSpaceButton;
    
    IBOutlet NSWindow *window;
    
    void *contextPtr;
    
    ControllerSource source;
    NSString *configuration;
    NSString *customPath;
    ControllerAction action;
    NSString *inputSpace;
    NSString *outputSpace;
    NSString *device;
    NSString *transform;
    
    ControllerInterp interpolation;
    BOOL invert;
}

- (id)initWithSource:(ControllerSource)source
        configuration:(NSString *)configuration
        action:(ControllerAction)action
        invert:(BOOL)invert
        interpolation:(ControllerInterp)interpolation
        inputSpace:(NSString *)inputSpace
        outputSpace:(NSString *)outputSpace
        device:(NSString *)device
        transform:(NSString *)transform;
        
- (IBAction)clickedOK:(id)sender;
- (IBAction)clickedCancel:(id)sender;
- (IBAction)clickedExport:(id)sender;

- (IBAction)trackConfigMenu:(id)sender;
- (IBAction)trackActionRadios:(id)sender;
- (IBAction)trackMenu1:(id)sender;
- (IBAction)trackMenu2:(id)sender;
- (IBAction)trackMenu3:(id)sender;
- (IBAction)trackInvert:(id)sender;

- (IBAction)popInputSpaceMenu:(id)sender;
- (IBAction)popOutputSpaceMenu:(id)sender;

- (NSWindow *)window;

- (ControllerSource)source;
- (NSString *)configuration;
- (ControllerAction)action;
- (BOOL)invert;
- (ControllerInterp)interpolation;
- (NSString *)inputSpace;
- (NSString *)outputSpace;
- (NSString *)device;
- (NSString *)transform;

@end
