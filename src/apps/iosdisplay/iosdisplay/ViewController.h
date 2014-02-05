//
//  ViewController.h
//  iosdisplay
//
//  Created by Brian Hall on 7/10/12.
//  Copyright (c) 2012 Sony Pictures Imageworks. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import "OverlayViewController.h"

@interface ViewController : GLKViewController <OverlayViewControllerDelegate>
{
    OverlayViewController *_overlayViewController;
}

@end
