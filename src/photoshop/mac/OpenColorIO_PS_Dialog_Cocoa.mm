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


#include "OpenColorIO_PS_Dialog.h"

#import "OpenColorIO_PS_Dialog_Controller.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "OpenColorIO_PS_Version.h"

// ==========
// Only building this on 64-bit (Cocoa) architectures
// ==========
#if __LP64__ || defined(COCOA_ON_32BIT)

DialogResult OpenColorIO_PS_Dialog(DialogParams &params, const void *plugHndl, const void *mwnd)
{
    DialogResult result = RESULT_OK;
    
    NSApplicationLoad();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    
    NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

    Class ui_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
                                    classNamed:@"OpenColorIO_PS_Dialog_Controller"];

    if(ui_controller_class)
    {
        ControllerSource source = (params.source == SOURCE_ENVIRONMENT ? CSOURCE_ENVIRONMENT :
                                     params.source == SOURCE_CUSTOM ? CSOURCE_CUSTOM:
                                     CSOURCE_STANDARD);
                                     
        ControllerAction action = (params.action == ACTION_LUT ? CACTION_LUT :
                                    params.action == ACTION_DISPLAY ? CACTION_DISPLAY :
                                    CACTION_CONVERT);
                                        
        ControllerInterp interpolation = (params.interpolation == INTERPO_NEAREST ? CINTERP_NEAREST :
                                            params.interpolation == INTERPO_LINEAR ? CINTERP_LINEAR :
                                            params.interpolation == INTERPO_TETRAHEDRAL ? CINTERP_TETRAHEDRAL :
                                            CINTERP_BEST);
                                                
        NSString *configuration = (params.config.empty() ? nil : [NSString stringWithUTF8String:params.config.c_str()]);
        const BOOL invert = params.invert;
        NSString *inputSpace = (params.inputSpace.empty() ? nil : [NSString stringWithUTF8String:params.inputSpace.c_str()]);
        NSString *outputSpace = (params.outputSpace.empty() ? nil : [NSString stringWithUTF8String:params.outputSpace.c_str()]);
        NSString *transform = (params.transform.empty() ? nil : [NSString stringWithUTF8String:params.transform.c_str()]);
        NSString *device = (params.device.empty() ? nil : [NSString stringWithUTF8String:params.device.c_str()]);
        
        
        OpenColorIO_PS_Dialog_Controller *controller = [[ui_controller_class alloc] initWithSource:source
                                                                                    configuration:configuration
                                                                                    action:action
                                                                                    invert:invert
                                                                                    interpolation:interpolation
                                                                                    inputSpace:inputSpace
                                                                                    outputSpace:outputSpace
                                                                                    device:device
                                                                                    transform:transform];
        if(controller)
        {
            NSWindow *window = [controller window];
        
            const NSUInteger modalResult = [NSApp runModalForWindow:window];
            
            if(modalResult == NSRunStoppedResponse)
            {
                source = [controller source];
                action = [controller action];
                interpolation = [controller interpolation];
                
                params.source = (source == CSOURCE_ENVIRONMENT ? SOURCE_ENVIRONMENT :
                                    source == CSOURCE_CUSTOM ? SOURCE_CUSTOM :
                                    SOURCE_STANDARD);
                
                params.action = (action == CACTION_LUT ? ACTION_LUT :
                                    action == CACTION_DISPLAY ? ACTION_DISPLAY :
                                    ACTION_CONVERT);
                                    
                params.interpolation = (interpolation == CINTERP_NEAREST ? INTERPO_NEAREST :
                                        interpolation == CINTERP_LINEAR ? INTERPO_LINEAR :
                                        interpolation == CINTERP_TETRAHEDRAL ? INTERPO_TETRAHEDRAL :
                                        INTERPO_BEST);
                
                if([controller configuration] != nil)
                    params.config = [[controller configuration] UTF8String];
                
                params.invert = [controller invert];
                
                if([controller inputSpace] != nil)
                    params.inputSpace = [[controller inputSpace] UTF8String];
                    
                if([controller outputSpace] != nil)
                    params.outputSpace = [[controller outputSpace] UTF8String];
                    
                if([controller transform] != nil)
                    params.transform = [[controller transform] UTF8String];
                    
                if([controller device] != nil)
                    params.device = [[controller device] UTF8String];
            
                result = RESULT_OK;
            }
            else
                result = RESULT_CANCEL;
            
            [window close];
            
            [controller release];
        }
    }
    
    
    [pool release];
    
    return  result;
}


void OpenColorIO_PS_About(const void *plugHndl, const void *mwnd)
{
    NSApplicationLoad();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    const std::string endl = "\n";

    std::string text = __DATE__ + endl +
                        endl +
                        "OCIO version " + OCIO::GetVersion();
                        
    NSString *informativeText = [NSString stringWithUTF8String:text.c_str()];
                        
    NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", informativeText];
    
    [alert setAlertStyle:NSInformationalAlertStyle];
    
    [alert runModal];
    
    [pool release];
}

#endif // __LP64__
