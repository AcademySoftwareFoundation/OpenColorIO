//
//  AppDelegate.m
//  iosdisplay
//
//  Created by Brian Hall on 7/10/12.
//  Copyright (c) 2012 Sony Pictures Imageworks. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"

@implementation AppDelegate

@synthesize window = _window;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

    ViewController *viewContoller= [[ViewController alloc] init];
    [self.window addSubview:viewContoller.view];
    _viewController = viewContoller;
    
    [self.window makeKeyAndVisible];
    return YES;
}

@end
