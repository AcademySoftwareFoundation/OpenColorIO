//
//  OpenColorIO_AE_MonitorProfileChooser_Controller.m
//
//  Created by Brendan Bolles on 11/25/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import "OpenColorIO_AE_MonitorProfileChooser_Controller.h"


typedef struct {
	NSMutableArray *path_array;
	NSMutableArray *name_array;
} IterateData;

static OSErr
profIterateProc(CMProfileIterateData* data, void* refcon)
{
	OSErr err = noErr;
	
	IterateData *i_data = (IterateData *)refcon;
	
	if(data->header.dataColorSpace == cmRGBData && data->location.locType == cmPathBasedProfile)
	{
		[i_data->path_array addObject:[NSString stringWithUTF8String:data->location.u.pathLoc.path]];
		
		[i_data->name_array addObject:[NSString stringWithUTF8String:data->asciiName] ];
	}
	
	return err;
}



@implementation OpenColorIO_AE_MonitorProfileChooser_Controller

- (id)init {
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"OpenColorIO_AE_MonitorProfileChooser" owner:self]))
		return nil;
	
	path_array = [[NSMutableArray alloc] init];
	name_array = [[NSMutableArray alloc] init];
	
	
	// build profile arrays
	IterateData i_data = { path_array, name_array };
	
    UInt32 seed = 0;
    UInt32 count;
	
	CMProfileIterateUPP iterateUPP;
	iterateUPP = NewCMProfileIterateUPP((CMProfileIterateProcPtr)&profIterateProc);

	CMError err = CMIterateColorSyncFolder(iterateUPP, &seed, &count, (void *)&i_data);
		
	DisposeCMProfileIterateUPP(iterateUPP);
	
	
	// set menu items with tags
	// the tag tells us the index of the menu item in the arrays
	int i;
	
	for(i=0; i < [name_array count]; i++)
	{
		NSString *title = [name_array objectAtIndex:i];
	
		[profileMenu addItemWithTitle:title];
		
		[[profileMenu itemWithTitle] setTag:i];
	}
	
	
	
	// set menu to monitor profile
	CMProfileRef prof;
	CMProfileLocation profLoc;
	
	UInt32 locationSize = cmCurrentProfileLocationSize;

	// Get the main GDevice.
	CGDirectDisplayID theAVID = CGMainDisplayID();

	// Get the profile for that AVID.
	err = CMGetProfileByAVID(theAVID, &prof);
	
	// Get location (FSRef) for that profile
	err = NCMGetProfileLocation(prof, &profLoc, &locationSize);
	
	if(profLoc.locType == cmPathBasedProfile)
	{
		BOOL found = NO;
		
		for(i=0; i < [path_array count]; i++)
		{
			NSString *icc_path = [path_array objectAtIndex:i];
			NSString *icc_name = [name_array objectAtIndex:i];
			
			if(icc_path && 0 == [icc_path compare:[NSString stringWithUTF8String:profLoc.u.pathLoc.path]])
			{
				NSInteger index = [profileMenu indexOfItemWithTag:i];
				
				//[profileMenu selectItemAtIndex:index];
				[profileMenu selectItemWithTitle:icc_name];
				
				found = YES;
			}
		}
		
		if(!found)
		{
			CFStringRef name;
			
			err = CMCopyProfileDescriptionString(prof, &name);
			
			NSString *ns_name = (NSString *)name;
			
			[path_array addObject:[NSString stringWithUTF8String:profLoc.u.pathLoc.path]];
			
			[name_array addObject:[NSString stringWithString:ns_name] ];
			
			
			NSInteger new_item_tag = [path_array count] - 1;
			
			[profileMenu addItemWithTitle:ns_name];
			
			[[profileMenu lastItem] setTag:new_item_tag];
			
			[profileMenu selectItemWithTag:new_item_tag];
			
			
			CFRelease(name);
		}
	}
	
	return self;
}

- (void)dealloc {
	[path_array release];
	[name_array release];
	
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
	NSInteger index = [[profileMenu selectedItem] tag];

	return [[path_array objectAtIndex:index] getCString:path maxLength:buf_len encoding:NSMacOSRomanStringEncoding];
}

@end
