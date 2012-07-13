//
//  OverlayViewController.m
//  iosdisplay
//
//  Created by Brian Hall on 7/12/12.
//  Copyright (c) 2012 Lonely Star Software. All rights reserved.
//

#import "OverlayViewController.h"

@implementation OverlayViewController

@synthesize exposure = _exposure;
@synthesize enableOCIO = _enableOCIO;
@synthesize delegate = _delegate;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        _exposure = 0.0f;
        _enableOCIO = YES;
        _delegate = nil;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor clearColor];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (void)updateExposureLabel
{
    _exposureLabel.text = [NSString stringWithFormat:@"Exposure: %0.1f", _exposure];
}

- (IBAction)exposureReset:(id)sender
{
    _exposure = 0.0f;
    _exposureSlider.value = _exposure;
    [self updateExposureLabel];
    [_delegate overlayViewControllerChanged:self];
}

- (IBAction)exposureSlider:(id)sender
{
    _exposure = _exposureSlider.value;
    [self updateExposureLabel];
    [_delegate overlayViewControllerChanged:self];
}

- (IBAction)OCIOSwitch:(id)sender
{
    _enableOCIO = _OCIOSwitch.on;
    [_delegate overlayViewControllerChanged:self];
}

@end
