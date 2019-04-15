#ifndef ViewCon_h__
#define ViewCon_h__

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#import <UIKit/UIKit.h>
#define PlatformViewController UIViewController
#else
#import <AppKit/AppKit.h>
#define PlatformViewController NSViewController
#endif

#import <MetalKit/MetalKit.h>
@class Renderer;

@interface ViewCon : PlatformViewController

@property(nonatomic,strong) MTKView *Mview;
@property(nonatomic,strong) Renderer *renderer;

- (void)viewDidLoad;
@end

#endif
