/*
Copyright (c) 2003-2017 Sony Pictures Imageworks Inc., et al.
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
