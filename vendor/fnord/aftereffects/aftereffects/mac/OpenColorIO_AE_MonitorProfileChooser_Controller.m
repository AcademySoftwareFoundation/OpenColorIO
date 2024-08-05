// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#import "OpenColorIO_AE_MonitorProfileChooser_Controller.h"


typedef struct {
    NSMutableArray<NSString *>                  *name_array;
    NSMutableDictionary<NSString *, NSURL *>    *profile_dict;
} IterateData;

static bool ProfileIterateCallback(CFDictionaryRef profileInfo, void* userInfo)
{
    IterateData *i_data = (IterateData *)userInfo;
    
    NSDictionary *profileDict = (__bridge NSDictionary *)profileInfo;
    
    NSString *profileDescription = [profileDict objectForKey:(__bridge NSString *)kColorSyncProfileDescription];
    NSURL *profileURL = [profileDict objectForKey:(__bridge NSString *)kColorSyncProfileURL];
    NSString *profileColorSpace = [profileDict objectForKey:(__bridge NSString *)kColorSyncProfileColorSpace];
    
    if([profileDescription isKindOfClass:[NSString class]] && [profileURL isKindOfClass:[NSURL class]] &&
        [profileURL isFileURL] && [profileColorSpace isEqualToString:(__bridge NSString *)kColorSyncSigRgbData] &&
        [i_data->name_array indexOfObjectIdenticalTo:profileDescription] == NSNotFound)
    {
        [i_data->name_array addObject:profileDescription];
        [i_data->profile_dict setObject:profileURL forKey:profileDescription];
    }
    
    return true;
}


@implementation OpenColorIO_AE_MonitorProfileChooser_Controller

- (id)init
{
    self = [super init];
    
    if(!([NSBundle loadNibNamed:@"OpenColorIO_AE_MonitorProfileChooser" owner:self]))
        return nil;
    
    
    [self.window center];
    
    // Originally tried to implement this with two NSArrays, one with paths and
    // one with profile names (ICC descriptions).  The problem is that when you
    // add items to NSArrays one at a time, they auto-sort.  But then it turns out I
    // WANT them to sort because the profiles come in random order and the menu looks
    // terrible if they're not sorted.
    
    // So I make an NSArray to set up the menu and an NSMutableDictionary to convert from the
    // selected menu item to the actual path.  Got that?
    
    
    NSMutableArray<NSString *> *name_array  = [NSMutableArray array];
    
    profile_dict = [[NSMutableDictionary alloc] init];
    
    
    // get monitor profile name
    
    NSString *defaultProfileDescription = nil;

    ColorSyncProfileRef displayProfileRef = ColorSyncProfileCreateWithDisplayID(0);
    
    if(displayProfileRef != NULL)
    {
        CFStringRef displayProfileNameRef = ColorSyncProfileCopyDescriptionString(displayProfileRef);
        
        if(displayProfileNameRef != NULL)
        {
            defaultProfileDescription = [NSString stringWithString:(__bridge NSString *)displayProfileNameRef];
            
            CFRelease(displayProfileNameRef);
        }
        
        CFRelease(displayProfileRef);
    }

    
    // build profile dict and name array
    IterateData i_data = { name_array, profile_dict };
    
    ColorSyncIterateInstalledProfiles(ProfileIterateCallback, NULL, &i_data, NULL);
    
    
    // set up menu with array
    [self.profileMenu addItemsWithTitles:name_array];
    
    [self.profileMenu selectItemWithTitle:defaultProfileDescription];
    
    
    return self;
}

- (void)dealloc
{
    [profile_dict release];
    
    [super dealloc];
}

- (IBAction)clickOK:(id)sender
{
    [NSApp stopModal];
}

- (IBAction)clickCancel:(id)sender
{
    [NSApp abortModal];
}

- (BOOL)getMonitorProfile:(char *)path bufferSize:(int)buf_len
{
    NSString *icc_name = [[self.profileMenu selectedItem] title];
    NSURL *icc_url = [profile_dict objectForKey:icc_name];
    NSString *icc_path = [icc_url path];

    return [icc_path getCString:path maxLength:buf_len encoding:NSUTF8StringEncoding];
}

@end
