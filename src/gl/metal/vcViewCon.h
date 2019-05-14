#ifndef vcViewCon_h__
#define vcViewCon_h__

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
#import <UIKit/UIKit.h>
#define PlatformViewController UIViewController
#elif UDPLATFORM_OSX
#import <AppKit/AppKit.h>
#define PlatformViewController NSViewController
#else
# error "Unsupported platform!"
#endif

#import <MetalKit/MetalKit.h>
@class vcRenderer;

@interface vcViewCon : PlatformViewController

@property(nonatomic,strong) MTKView *Mview;
@property(nonatomic,strong) vcRenderer *renderer;

- (void)viewDidLoad;
@end

#endif
