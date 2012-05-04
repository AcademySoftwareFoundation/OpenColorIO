/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
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

#import "OpenColorIO_AE_MonitorProfileChooser_Controller.h"


typedef struct {
    NSMutableArray  *name_array;
    NSMapTable      *profile_map;
    const char      *monitor_profile_path;
    char            *monitor_profile_name;
} IterateData;

static OSErr profIterateProc(CMProfileIterateData* data, void* refcon)
{
    OSErr err = noErr;
    
    IterateData *i_data = (IterateData *)refcon;
    
    if(data->header.dataColorSpace == cmRGBData && data->location.locType == cmPathBasedProfile)
    {
        [i_data->name_array addObject:[NSString stringWithUTF8String:data->asciiName]];
    
        [i_data->profile_map setObject:[NSString stringWithUTF8String:data->location.u.pathLoc.path]
                                forKey:[NSString stringWithUTF8String:data->asciiName] ];
        
        if( i_data->monitor_profile_path &&
            !strcmp(data->location.u.pathLoc.path, i_data->monitor_profile_path) )
        {
            strncpy(i_data->monitor_profile_name, data->asciiName, 255);
        }
    }
    
    return err;
}



@implementation OpenColorIO_AE_MonitorProfileChooser_Controller

- (id)init {
    self = [super init];
    
    if(!([NSBundle loadNibNamed:@"OpenColorIO_AE_MonitorProfileChooser" owner:self]))
        return nil;
    
    
    [window center];
    
    // Originally tried to implement this with two NSArrays, one with paths and
    // one with profile names (ICC descriptions).  The problem is that when you
    // add items to NSArrays one at a time, they auto-sort.  But then it turns out I
    // WANT them to sort because the profiles come in random order and the menu looks
    // terrible if they're not sorted.
    
    // So I make an NSArray to set up the menu and an NSMapTable to convert from the
    // selected menu item to the actual path.  Got that?
    
    
    name_array  = [[NSMutableArray alloc] init];
    profile_map = [[NSMapTable alloc] init];
    
    
    // get monitor profile path
    
    // Oddly enough, the "Name" given to me by ColorSync for this is often "Display",
    // but if you get the profile's description, you get something like
    // "Apple Cinema HD Display".  So to see if ColorSync runs accross the the monitor's
    // profile so I can select it, I have to compare the paths, and then save the name
    // I'm currently getting.
    
    CMProfileRef prof;
    CMProfileLocation profLoc;
    
    UInt32 locationSize = cmCurrentProfileLocationSize;

    // Get the main GDevice.
    CGDirectDisplayID theAVID = CGMainDisplayID();

    // Get the profile for that AVID.
    CMError err = CMGetProfileByAVID(theAVID, &prof);
    
    // Get location (FSRef) for that profile
    err = NCMGetProfileLocation(prof, &profLoc, &locationSize);
    
    const char *monitor_profile_path = NULL;
    char monitor_profile_name[256] = { '\0' };
    
    if(profLoc.locType == cmPathBasedProfile)
    {
        monitor_profile_path = profLoc.u.pathLoc.path;
    }
    
    
    
    // build profile map and name array
    IterateData i_data = { name_array, profile_map, monitor_profile_path, monitor_profile_name };
    
    UInt32 seed = 0;
    UInt32 count;
    
    CMProfileIterateUPP iterateUPP;
    iterateUPP = NewCMProfileIterateUPP((CMProfileIterateProcPtr)&profIterateProc);

    err = CMIterateColorSyncFolder(iterateUPP, &seed, &count, (void *)&i_data);
        
    DisposeCMProfileIterateUPP(iterateUPP);
    
    
    
    // set up menu with array
    [profileMenu addItemsWithTitles:name_array];
    
    
    
    // choose the display profile name if we have it (usually "Display")
    if(monitor_profile_name[0] != '\0')
    {
        [profileMenu selectItemWithTitle:[NSString stringWithUTF8String:monitor_profile_name]];
    }
    else if(monitor_profile_path != NULL)
    {
        // somehow the display profile wasn't found during iteration
        // so let's add it
        CFStringRef m_name;
        
        err = CMCopyProfileDescriptionString(prof, &m_name);
        
        NSString *ns_name = (NSString *)monitor_profile_name;
        
        [profile_map setObject:[NSString stringWithUTF8String:monitor_profile_path]
                        forKey:ns_name ];
        
        [profileMenu addItemWithTitle:ns_name];
        
        [profileMenu selectItemWithTitle:ns_name];
        
        CFRelease(m_name);
    }
    
    return self;
}

- (void)dealloc {
    [name_array release];
    [profile_map release];
    
    [super dealloc];
}

- (IBAction)clickOK:(NSButton *)sender {
    [NSApp stopModal];
}

- (IBAction)clickCancel:(NSButton *)sender {
    [NSApp abortModal];
}

- (NSWindow *)getWindow {
    return window;
}

- (BOOL)getMonitorProfile:(char *)path bufferSize:(int)buf_len {
    NSString *icc_name = [[profileMenu selectedItem] title];
    NSString *icc_path = [profile_map objectForKey:icc_name];

    return [icc_path getCString:path maxLength:buf_len encoding:NSMacOSRomanStringEncoding];
}

@end
