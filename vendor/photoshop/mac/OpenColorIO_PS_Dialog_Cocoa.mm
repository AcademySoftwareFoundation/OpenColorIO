// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
                                            params.interpolation == INTERPO_CUBIC ? CINTERP_CUBIC :
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
                                        interpolation == CINTERP_CUBIC ? INTERPO_CUBIC :
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
