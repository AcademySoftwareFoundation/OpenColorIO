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

#import "OpenColorIO_PS_Dialog_Controller.h"

#import "OpenColorIO_AE_MonitorProfileChooser_Controller.h"

#include <fstream>
#include <map>
#include <cstdlib>

#include "OpenColorIO_PS_Context.h"

#include "OpenColorIO_AE_Dialogs.h"

#include "ocioicc.h"



static NSString *standardPath = @"/Library/Application Support/OpenColorIO";


@implementation OpenColorIO_PS_Dialog_Controller

- (NSString *)pathForStandardConfig:(NSString *)config
{
    return [[standardPath stringByAppendingPathComponent:config] stringByAppendingPathComponent:@"config.ocio"];
}

- (id)initWithSource:(ControllerSource)initSource
        configuration:(NSString *)initConfiguration
        action:(ControllerAction)initAction
        invert:(BOOL)initInvert
        interpolation:(ControllerInterp)initInterpolation
        inputSpace:(NSString *)initInputSpace
        outputSpace:(NSString *)initOutputSpace
        device:(NSString *)initDevice
        transform:(NSString *)initTransform
{
    self = [super init];
    
    if(self)
    {
        if(!([NSBundle loadNibNamed:@"OpenColorIO_PS_Dialog" owner:self]))
            return nil;
        
        source = initSource;
        
        if(source == CSOURCE_CUSTOM)
        {
            customPath = [initConfiguration retain];
        }
        else if(source == CSOURCE_STANDARD)
        {
            configuration = [initConfiguration retain];
        }
        
        action = initAction;
        inputSpace = [initInputSpace retain];
        outputSpace = [initOutputSpace retain];
        device = [initDevice retain];
        transform = [initTransform retain];
        interpolation = initInterpolation;
        invert = initInvert;
        
        
        // configuration menu
        [configurationMenu removeAllItems];
        
        [configurationMenu setAutoenablesItems:NO];
        
        [configurationMenu addItemWithTitle:@"$OCIO"];
        
        [[configurationMenu lastItem] setTag:CSOURCE_ENVIRONMENT];
        
        char *envFile = std::getenv("OCIO");
        
        if(envFile == NULL || strlen(envFile) == 0)
            [[configurationMenu lastItem] setEnabled:FALSE];
        
        
        [[configurationMenu menu] addItem:[NSMenuItem separatorItem]];
        
        
        NSArray *standardConfigs = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:standardPath error:NULL];
        
        if([standardConfigs count] > 0)
        {
            for(NSString *configName in standardConfigs)
            {
                NSAssert([configName isKindOfClass:[NSString class]], @"expected NSString");
                
                NSString *configFilePath = [self pathForStandardConfig:configName];
                
                if([[NSFileManager defaultManager] isReadableFileAtPath:configFilePath])
                {
                    [configurationMenu addItemWithTitle:configName];
                    
                    [[configurationMenu lastItem] setTag:CSOURCE_STANDARD];
                }
            }
        }
        else
        {
            NSString *noConfigsMessage = [NSString stringWithFormat:@"No configs in %@", standardPath];
            
            [configurationMenu addItemWithTitle:noConfigsMessage];
            
            [[configurationMenu lastItem] setTag:CSOURCE_STANDARD];
            [[configurationMenu lastItem] setEnabled:FALSE];
        }

        
        [[configurationMenu menu] addItem:[NSMenuItem separatorItem]];
        
        
        [configurationMenu addItemWithTitle:@"Custom…"];
        
        [[configurationMenu lastItem] setTag:CSOURCE_CUSTOM];
        
        
        [invertCheck setHidden:YES];
        
        [menu1 removeAllItems];
        [menu2 removeAllItems];
        [menu3 removeAllItems];
        
        [label3 setHidden:YES];
        [menu3 setHidden:YES];
        
        
        [self trackConfigMenu:nil];
    }
    
    return self;
}

- (void)dealloc
{
    OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
    
    delete context;
    
    [configuration release];
    [customPath release];
    [inputSpace release];
    [outputSpace release];
    [device release];
    [transform release];
    
    [super dealloc];
}
        
- (IBAction)clickedOK:(id)sender
{
    [NSApp stopModal];
}

- (IBAction)clickedCancel:(id)sender
{
    [NSApp abortModal];
}

/*
- (void)profileChooserDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    NSAssert(profileController != nil, @"profileController is nil");
    
    
    [sheet orderOut:self];
    

    if(returnCode == NSRunStoppedResponse)
    {
        NSString *path = (NSString *)contextInfo;
        
        NSAssert([path isKindOfClass:[NSString class]], @"expected NSString");

        try
        {
            OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
        
            NSAssert(context != NULL, @"context was NULL");
            
            
            char display_icc_path[256];
        
            const BOOL gotICC = [profileController getMonitorProfile:display_icc_path bufferSize:255];
            
            if(!gotICC)
                throw OCIO::Exception("Failed to get ICC profile");

            
            OCIO::ConstProcessorRcPtr processor;
            
            if(action == CACTION_CONVERT)
            {
                processor = context->getConvertProcessor([inputSpace UTF8String], [outputSpace UTF8String]);
            }
            else if(action == CACTION_DISPLAY)
            {
                processor = context->getDisplayProcessor([inputSpace UTF8String], [device UTF8String], [transform UTF8String]);
            }
            else
            {
                NSAssert(action == CACTION_LUT, @"expected CACTION_LUT");
                
                const OCIO::Interpolation interp = (interpolation == CINTERP_NEAREST ? OCIO::INTERP_NEAREST :
                                                    interpolation == CINTERP_LINEAR ? OCIO::INTERP_LINEAR :
                                                    interpolation == CINTERP_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
                                                    OCIO::INTERP_BEST);
                
                const OCIO::TransformDirection direction = (invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
                
                processor = context->getLUTProcessor(interp, direction);
            }
            
            
            int cubesize = 32;
            int whitepointtemp = 6505;
            std::string copyright = "OpenColorIO, Sony Imageworks";
            
            // create a description tag from the filename
            std::string description = [[[path lastPathComponent] stringByDeletingPathExtension] UTF8String];
            
            SaveICCProfileToFile([path UTF8String], processor, cubesize, whitepointtemp,
                                    display_icc_path, description, copyright, false);
        }
        catch(const std::exception &e)
        {
            NSBeep();
        
            NSString *ocioString = [NSString stringWithUTF8String:e.what()];
        
            NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

            [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
        }
        catch(...)
        {
            NSBeep();
            
            NSString *ocioString = @"Some unknown error";
            
            NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

            [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
        }
        
        [path release];
    }
    
    
    [profileController release];
    
    profileController = nil;
}
*/
- (void)exportPanelDidEnd:(NSSavePanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    if(returnCode == NSOKButton)
    {
        OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
    
        NSAssert(context != NULL, @"context was NULL");

    
        NSString *path = [[panel URL] path];
        
        NSString *extension = [path pathExtension];
        
        if([extension isEqualToString:@"icc"])
        {
            OpenColorIO_AE_MonitorProfileChooser_Controller *profileController = [[OpenColorIO_AE_MonitorProfileChooser_Controller alloc] init];
            
            // the sheet is still active, so can't run another one...will settle for modal
            const NSInteger modal_result = [NSApp runModalForWindow:[profileController getWindow]];
            
                                    
            if(modal_result == NSRunStoppedResponse)
            {
                try
                {
                    char display_icc_path[256];
                
                    const BOOL gotICC = [profileController getMonitorProfile:display_icc_path bufferSize:255];
                    
                    if(!gotICC)
                        throw OCIO::Exception("Failed to get ICC profile");

                    
                    OCIO::ConstProcessorRcPtr processor;
                    
                    if(action == CACTION_CONVERT)
                    {
                        processor = context->getConvertProcessor([inputSpace UTF8String], [outputSpace UTF8String]);
                    }
                    else if(action == CACTION_DISPLAY)
                    {
                        processor = context->getDisplayProcessor([inputSpace UTF8String], [device UTF8String], [transform UTF8String]);
                    }
                    else
                    {
                        NSAssert(action == CACTION_LUT, @"expected CACTION_LUT");
                        
                        const OCIO::Interpolation interp = (interpolation == CINTERP_NEAREST ? OCIO::INTERP_NEAREST :
                                                            interpolation == CINTERP_LINEAR ? OCIO::INTERP_LINEAR :
                                                            interpolation == CINTERP_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
                                                            OCIO::INTERP_BEST);
                        
                        const OCIO::TransformDirection direction = (invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
                        
                        processor = context->getLUTProcessor(interp, direction);
                    }
                    
                    
                    int cubesize = 32;
                    int whitepointtemp = 6505;
                    std::string copyright = "OpenColorIO, Sony Imageworks";
                    
                    // create a description tag from the filename
                    std::string description = [[[path lastPathComponent] stringByDeletingPathExtension] UTF8String];
                    
                    SaveICCProfileToFile([path UTF8String], processor, cubesize, whitepointtemp,
                                            display_icc_path, description, copyright, false);
                }
                catch(const std::exception &e)
                {
                    NSBeep();
                
                    NSString *ocioString = [NSString stringWithUTF8String:e.what()];
                
                    NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

                    [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
                }
                catch(...)
                {
                    NSBeep();
                    
                    NSString *ocioString = @"Some unknown error";
                    
                    NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

                    [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
                }
            }
            
            [[profileController getWindow] close];
            
            [profileController release];
        }
        else
        {
            try
            {
                // need an extension->format map
                std::map<std::string, std::string> extensions;
                
                for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
                {
                    const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
                    const char *format = OCIO::Baker::getFormatNameByIndex(i);
                    
                    extensions[ extension ] = format;
                }
                
                const std::string the_extension = [extension UTF8String];
                
                std::string format = extensions[ the_extension ];
                
                
                OCIO::BakerRcPtr baker;
                
                if(action == CACTION_CONVERT)
                {
                    baker = context->getConvertBaker([inputSpace UTF8String], [outputSpace UTF8String]);
                }
                else if(action == CACTION_DISPLAY)
                {
                    baker = context->getDisplayBaker([inputSpace UTF8String], [device UTF8String], [transform UTF8String]);
                }
                else
                {
                    NSAssert(action == CACTION_LUT, @"expected CACTION_LUT");
                    
                    const OCIO::Interpolation interp = (interpolation == CINTERP_NEAREST ? OCIO::INTERP_NEAREST :
                                                        interpolation == CINTERP_LINEAR ? OCIO::INTERP_LINEAR :
                                                        interpolation == CINTERP_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
                                                        OCIO::INTERP_BEST);
                    
                    const OCIO::TransformDirection direction = (invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
                    
                    baker = context->getLUTBaker(interp, direction);
                }
                
                baker->setFormat( format.c_str() );
                
                std::ofstream f([path UTF8String]);
                baker->bake(f);
            }
            catch(const std::exception &e)
            {
                NSBeep();
            
                NSString *ocioString = [NSString stringWithUTF8String:e.what()];
            
                NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

                [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
            }
            catch(...)
            {
                NSBeep();
                
                NSString *ocioString = @"Some unknown error";
                
                NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

                [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
            }
        }
    }
}

- (IBAction)clickedExport:(id)sender
{
    NSSavePanel *panel = [NSSavePanel savePanel];
    
    NSMutableString *message = [NSMutableString stringWithString:@"Formats: ICC Profile (.icc)"];
    
    NSMutableArray *extensionArray = [NSMutableArray arrayWithObject:@"icc"];
    
    for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
    {
        const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
        const char *format = OCIO::Baker::getFormatNameByIndex(i);
        
        [extensionArray addObject:[NSString stringWithUTF8String:extension]];
        
        [message appendFormat:@", %s (.%s)", format, extension];
    }
    
    [panel setAllowedFileTypes:extensionArray];
    [panel setMessage:message];
    
    [panel beginSheetForDirectory:nil
        file:nil
        modalForWindow:window
        modalDelegate:self
        didEndSelector:@selector(exportPanelDidEnd:returnCode:contextInfo:)
        contextInfo:NULL];
}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    if(returnCode == NSOKButton)
    {
        source = CSOURCE_CUSTOM;
    
        NSURL *url = [panel URL];
        
        [customPath release];
        
        customPath = [[url path] retain];
    }
        
    [self trackConfigMenu:nil];
}

- (IBAction)trackConfigMenu:(id)sender
{
    const ControllerSource previousSource = source;

    if(sender == nil)
    {
        // set menu from values
        if(source == CSOURCE_STANDARD)
        {
            [configurationMenu selectItemWithTitle:configuration];
        }
        else
        {
            [configurationMenu selectItemWithTag:source];
        }
    }
    else
    {
        // set value from menu
        source = (ControllerSource)[[configurationMenu selectedItem] tag];
    }


    NSString *configPath = nil;
    
    if(source == CSOURCE_ENVIRONMENT)
    {
        char *envFile = std::getenv("OCIO");
        
        if(envFile != NULL && strlen(envFile) > 0)
        {
            configPath = [NSString stringWithUTF8String:envFile];
        }
    }
    else if(source == CSOURCE_CUSTOM)
    {
        if(sender != nil)
        {
            source = previousSource; // we'll re-set to the previous source in case dialog is canceled
        
            // do file open dialog
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            
            [panel setCanChooseDirectories:NO];
            [panel setAllowsMultipleSelection:NO];
            
            NSMutableArray *extensions = [NSMutableArray arrayWithObject:@"ocio"];
            
            for(int i=0; i < OCIO::FileTransform::getNumFormats(); ++i)
            {
                const char *extension = OCIO::FileTransform::getFormatExtensionByIndex(i);
                //const char *format = OCIO::FileTransform::getFormatNameByIndex(i);
                
                NSString *extensionString = [NSString stringWithUTF8String:extension];
                
                if(![extensionString isEqualToString:@"ccc"]) // .ccc files require an ID parameter
                    [extensions addObject:[NSString stringWithUTF8String:extension]];
            }
            
            
            NSMutableString *message = [NSMutableString stringWithString:@"Formats: "];
            
            for(int i=0; i < [extensions count]; i++)
            {
                NSString *extension = [extensions objectAtIndex:i];
                
                if(i != 0)
                    [message appendString:@", "];
                
                [message appendString:@"."];
                [message appendString:extension];
            }
            
            [panel setMessage:message];
            [panel setAllowedFileTypes:extensions];
            
            [panel beginSheetForDirectory:nil
                    file:nil
                    modalForWindow:window
                    modalDelegate:self
                    didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                    contextInfo:NULL];
            
            return;
        }
        else
        {
            configPath = customPath;
        }
    }
    else
    {
        NSAssert(source == CSOURCE_STANDARD, @"expected TAG_STANDARD");
        
        [configuration release];
        
        configuration = [[configurationMenu titleOfSelectedItem] retain];
        
        configPath = [self pathForStandardConfig:configuration];
    }
    
    
    if(configPath != nil && [[NSFileManager defaultManager] isReadableFileAtPath:configPath])
    {
        try
        {
            OpenColorIO_PS_Context *oldContext = (OpenColorIO_PS_Context *)contextPtr;
            
            delete oldContext;
            
        
            OpenColorIO_PS_Context *context = new OpenColorIO_PS_Context([configPath UTF8String]);
            
            contextPtr = context;
            
            
            if( context->isLUT() )
            {
                action = CACTION_LUT;
                
                [invertCheck setHidden:NO];
                
                if( context->canInvertLUT() )
                {
                    [invertCheck setEnabled:YES];
                }
                else
                {
                    [invertCheck setEnabled:NO];
                
                    invert = NO;
                }
                
                [invertCheck setState:(invert ? NSOnState : NSOffState)];
                
                [actionRadios setHidden:YES];
                
                
                NSTextField *interpolationLabel = label1;
                NSPopUpButton *interpolationMenu = menu1;
                
                [interpolationLabel setStringValue:@"Interpolation"];
                
                // interpolation menu
                [interpolationMenu removeAllItems];
                
                [interpolationMenu addItemWithTitle:@"Nearest Neighbor"];
                [[interpolationMenu lastItem] setTag:CINTERP_NEAREST];
                
                [interpolationMenu addItemWithTitle:@"Linear"];
                [[interpolationMenu lastItem] setTag:CINTERP_LINEAR];
                
                [interpolationMenu addItemWithTitle:@"Tetrahedral"];
                [[interpolationMenu lastItem] setTag:CINTERP_TETRAHEDRAL];
                
                const bool canTetrahedral = !context->canInvertLUT();
                
                if(!canTetrahedral)
                {
                    [interpolationMenu setAutoenablesItems:NO];
                
                    [[interpolationMenu lastItem] setEnabled:NO];
                    
                    if(interpolation == CINTERP_TETRAHEDRAL)
                        interpolation = CINTERP_LINEAR;
                }
                
                [[interpolationMenu menu] addItem:[NSMenuItem separatorItem]];
                
                [interpolationMenu addItemWithTitle:@"Best"];
                [[interpolationMenu lastItem] setTag:CINTERP_BEST];
                
                [interpolationMenu selectItemWithTag:interpolation];
                
                [inputSpaceButton setHidden:YES];
                
                [label2 setHidden:YES];
                [menu2 setHidden:YES];
                [outputSpaceButton setHidden:YES];
                
                [label3 setHidden:YES];
                [menu3 setHidden:YES];
            }
            else
            {
                [invertCheck setHidden:YES];
            
                if(action == CACTION_LUT)
                {
                    action = CACTION_CONVERT;
                }
                
                [actionRadios setHidden:NO];
                                
                
                const SpaceVec &colorSpaces = context->getColorSpaces();
                
                if(inputSpace == nil || -1 == FindSpace(colorSpaces, [inputSpace UTF8String]))
                {
                    [inputSpace release];
                    
                    inputSpace = [[NSString alloc] initWithUTF8String:context->getDefaultColorSpace().c_str()];
                }
                
                if(outputSpace == nil || -1 == FindSpace(colorSpaces, [outputSpace UTF8String]))
                {
                    [outputSpace release];
                    
                    outputSpace = [[NSString alloc] initWithUTF8String:context->getDefaultColorSpace().c_str()];
                }
                
                
                const SpaceVec &devices = context->getDevices();
                
                if(device == nil || -1 == FindSpace(devices, [device UTF8String]))
                {
                    [device release];
                    
                    device = [[NSString alloc] initWithUTF8String:context->getDefaultDevice().c_str()];
                }
                
                
                const SpaceVec transforms = context->getTransforms([device UTF8String]);
                
                if(transform == nil || -1 == FindSpace(transforms, [transform UTF8String]))
                {
                    [transform release];
                    
                    transform = [[NSString alloc] initWithUTF8String:context->getDefaultTransform([device UTF8String]).c_str()];
                }
                
                
                [self trackActionRadios:nil];
            }
        }
        catch(const std::exception &e)
        {
            NSBeep();
        
            NSString *ocioString = [NSString stringWithUTF8String:e.what()];
        
            NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

            [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];

            if(source != CSOURCE_ENVIRONMENT)
            {
                source = CSOURCE_ENVIRONMENT;
                
                [self trackConfigMenu:nil];
            }
        }
        catch(...)
        {
            NSBeep();
            
            NSString *ocioString = @"Some unknown error";
        
            NSAlert *alert = [NSAlert alertWithMessageText:@"OpenColorIO error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", ocioString];

            [alert beginSheetModalForWindow:window modalDelegate:nil didEndSelector:NULL contextInfo:NULL];

            if(source != CSOURCE_ENVIRONMENT)
            {
                source = CSOURCE_ENVIRONMENT;
                
                [self trackConfigMenu:nil];
            }
        }
    }
            
    [configurationMenu setToolTip:configPath];
}

- (IBAction)trackActionRadios:(id)sender;
{
    if(sender == nil)
    {
        // set radios from values
        NSAssert(action != CACTION_LUT, @"should not be a LUT");
        
        const NSUInteger selectedColumn = (action == CACTION_DISPLAY ? 1 : 0);
        
        [actionRadios selectCellAtRow:0 column:selectedColumn];
    }
    else
    {
        // set values from radios
        action = ([actionRadios selectedColumn] == 1 ? CACTION_DISPLAY : CACTION_CONVERT);
    }
    
    
    OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
    
    NSAssert(context != NULL, @"context was NULL");
    
    
    NSTextField *inputLabel = label1;
    NSPopUpButton *inputMenu = menu1;
    
    [inputLabel setStringValue:@"Input Space:"];
    
    [inputMenu removeAllItems];
    
    const SpaceVec &colorSpaces = context->getColorSpaces();
    
    for(SpaceVec::const_iterator i = colorSpaces.begin(); i != colorSpaces.end(); ++i)
    {
        NSString *colorSpace = [NSString stringWithUTF8String:i->c_str()];
        
        [inputMenu addItemWithTitle:colorSpace];
    }
    
    NSAssert([inputMenu itemWithTitle:inputSpace] != nil, @"don't have the input space");
    
    [inputMenu selectItemWithTitle:inputSpace];
    
    [inputSpaceButton setHidden:NO];
    
    
    if(action == CACTION_DISPLAY)
    {
        NSTextField *deviceLabel = label2;
        NSPopUpButton *deviceMenu = menu2;
        
        NSTextField *transformLabel = label3;
        NSPopUpButton *transformMenu = menu3;
        
    
        [deviceLabel setStringValue:@"Device:"];
        [deviceLabel setHidden:NO];
        
        [deviceMenu setHidden:NO];
        
        [deviceMenu removeAllItems];
        
        const SpaceVec &devices = context->getDevices();
        
        for(SpaceVec::const_iterator i = devices.begin(); i != devices.end(); ++i)
        {
            NSString *deviceName = [NSString stringWithUTF8String:i->c_str()];
            
            [deviceMenu addItemWithTitle:deviceName];
        }
        
        [outputSpaceButton setHidden:YES];
                
        
        [transformLabel setStringValue:@"Transform:"];
        [transformLabel setHidden:NO];
        
        [transformMenu setHidden:NO];
        
        
        [self trackMenu2:nil];
    }
    else
    {
        NSAssert(action == CACTION_CONVERT, @"expected Convert");
        
        NSTextField *outputLabel = label2;
        NSPopUpButton *outputMenu = menu2;
        
        [outputLabel setHidden:NO];
        [outputLabel setStringValue:@"Output Space:"];
        
        [outputMenu setHidden:NO];
        
        [outputMenu removeAllItems];
        
        for(SpaceVec::const_iterator i = colorSpaces.begin(); i != colorSpaces.end(); ++i)
        {
            NSString *colorSpace = [NSString stringWithUTF8String:i->c_str()];
            
            [outputMenu addItemWithTitle:colorSpace];
        }
        
        NSAssert([outputMenu itemWithTitle:outputSpace] != nil, @"don't have the input space");
        
        [outputMenu selectItemWithTitle:outputSpace];
        
        [outputSpaceButton setHidden:NO];
        
        
        NSTextField *transformLabel = label3;
        NSPopUpButton *transformMenu = menu3;
        
        [transformLabel setHidden:YES];
        [transformMenu setHidden:YES];
        
        NSAssert([[transformLabel stringValue] isEqualToString:@"Transform:"], @"expected Transform:");
    }
}

- (IBAction)trackMenu1:(id)sender
{
    NSAssert(sender == menu1, @"always from the UI");
    
    if(action == CACTION_LUT)
    {
        interpolation = (ControllerInterp)[[menu1 selectedItem] tag];
    }
    else
    {
        [inputSpace release];
        
        inputSpace = [[menu1 titleOfSelectedItem] retain];
    }
}

- (IBAction)trackMenu2:(id)sender
{
    if(action == CACTION_DISPLAY)
    {
        NSPopUpButton *deviceMenu = menu2;
        
        if(sender == nil)
        {
            // set menu from values
            NSAssert([deviceMenu itemWithTitle:device] != nil, @"don't have the device");
            
            [deviceMenu selectItemWithTitle:device];
        }
        else
        {
            // set values from menu
            [device release];
            
            device = [[deviceMenu titleOfSelectedItem] retain];
        }
    }
    else
    {
        NSAssert(action == CACTION_CONVERT, @"expected Convert");
        
        NSPopUpButton *outputMenu = menu2;
        
        if(sender == nil)
        {
            // set menu from values
            NSAssert([outputMenu itemWithTitle:outputSpace] != nil, @"don't have the device");
            
            [outputMenu selectItemWithTitle:outputSpace];
        }
        else
        {
            // set values from menu
            [outputSpace release];
            
            outputSpace = [[outputMenu titleOfSelectedItem] retain];
        }
    }
    
    
    if(action == CACTION_DISPLAY)
    {
        OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
        
        NSAssert(context != NULL, @"context was NULL");
        
        const SpaceVec transforms = context->getTransforms([device UTF8String]);
        
        NSPopUpButton *transformMenu = menu3;
        
        [transformMenu removeAllItems];
        
        for(SpaceVec::const_iterator i = transforms.begin(); i != transforms.end(); ++i)
        {
            NSString *transformName = [NSString stringWithUTF8String:i->c_str()];
            
            [transformMenu addItemWithTitle:transformName];
        }
        
        [self trackMenu3:nil];
    }
}

- (IBAction)trackMenu3:(id)sender
{
    NSAssert(action == CACTION_DISPLAY, @"expected Display");
    NSAssert([[label3 stringValue] isEqualToString:@"Transform:"], @"expected Transform:");
    
    NSPopUpButton *transformMenu = menu3;

    if(sender == nil)
    {
        // set menu from value
        NSMenuItem *valueItem = [transformMenu itemWithTitle:transform];
        
        if(valueItem != nil)
        {
            [transformMenu selectItem:valueItem];
        }
        else
        {
            OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
            
            NSAssert(context != NULL, @"context was NULL");
            
            const std::string defaultTransform = context->getDefaultTransform([device UTF8String]);
            
            NSMenuItem *defaultItem = [transformMenu itemWithTitle:[NSString stringWithUTF8String:defaultTransform.c_str()]];
            
            NSAssert(defaultItem != nil, @"where's that default item?");
            
            [transformMenu selectItem:defaultItem];
            
            
            [transform release];
            
            transform = [[transformMenu titleOfSelectedItem] retain];
        }
    }
    else
    {
        // set value from menu
        [transform release];
        
        transform = [[transformMenu titleOfSelectedItem] retain];
    }
}

- (IBAction)trackInvert:(id)sender
{
    NSAssert(sender == invertCheck, @"expected invertCheck");
    
    invert = ([invertCheck state] == NSOnState);
}

- (IBAction)popInputSpaceMenu:(id)sender
{
    OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
    
    if(context != NULL)
    {
        std::string colorSpace = [inputSpace UTF8String];
    
        const bool chosen = ColorSpacePopUpMenu(context->getConfig(), colorSpace, false, NULL);
        
        if(chosen)
        {
            [inputSpace release];
        
            inputSpace = [[NSString alloc] initWithUTF8String:colorSpace.c_str()];
            
            [menu1 selectItemWithTitle:inputSpace];
        }
    }
}

- (IBAction)popOutputSpaceMenu:(id)sender
{
    OpenColorIO_PS_Context *context = (OpenColorIO_PS_Context *)contextPtr;
    
    if(context != NULL)
    {
        std::string colorSpace = [outputSpace UTF8String];
    
        const bool chosen = ColorSpacePopUpMenu(context->getConfig(), colorSpace, false, NULL);
        
        if(chosen)
        {
            [outputSpace release];
        
            outputSpace = [[NSString alloc] initWithUTF8String:colorSpace.c_str()];
            
            [menu2 selectItemWithTitle:outputSpace];
        }
    }
}

- (NSWindow *)window
{
    return window;
}

- (ControllerSource)source
{
    return source;
}

- (NSString *)configuration
{
    if(source == CSOURCE_CUSTOM)
    {
        return customPath;
    }
    else
    {
        return configuration;
    }
}

- (ControllerAction)action
{
    return action;
}

- (BOOL)invert
{
    return invert;
}

- (ControllerInterp)interpolation
{
    return interpolation;
}

- (NSString *)inputSpace
{
    return inputSpace;
}

- (NSString *)outputSpace
{
    return outputSpace;
}

- (NSString *)device
{
    return device;
}

- (NSString *)transform
{
    return transform;
}

@end
