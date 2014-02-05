//
//  OverlayViewController.h
//  iosdisplay
//
//  Created by Brian Hall on 7/12/12.
//  Copyright (c) 2012 Lonely Star Software. All rights reserved.
//

#import <UIKit/UIKit.h>

@class OverlayViewController;

@protocol OverlayViewControllerDelegate
- (void)overlayViewControllerChanged:(OverlayViewController *)overlayViewController;
@end

@interface OverlayViewController : UIViewController
{
    CGFloat _exposure;
    BOOL _enableOCIO;
    
    IBOutlet UILabel *_exposureLabel;
    IBOutlet UISlider *_exposureSlider;
    IBOutlet UIButton *_exposureReset;
    IBOutlet UISwitch *_OCIOSwitch;
    
    __unsafe_unretained id <OverlayViewControllerDelegate> _delegate;
}

@property (readonly) CGFloat exposure;
@property (readonly) BOOL enableOCIO;
@property (assign) IBOutlet id <OverlayViewControllerDelegate> delegate;

- (IBAction)exposureReset:(id)sender;
- (IBAction)exposureSlider:(id)sender;
- (IBAction)OCIOSwitch:(id)sender;

@end
