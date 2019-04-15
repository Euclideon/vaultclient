#import <Metal/Metal.h>
#import <MetalKit/MTKView.h>
#import "vcMetal.h"
#import "ViewCon.h"
#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

@implementation ViewCon

- (void)viewDidLoad
{
    [super viewDidLoad];

    _renderer = [[Renderer alloc] initWithMetalKitView:_Mview];
#ifdef METAL_DEBUG
    if(!_renderer)
    {
        NSLog(@"Renderer failed initialization");
    }
#endif
    // Per framebuffer objects
    _renderer.commandBuffers = [NSMutableArray arrayWithCapacity:BUFFER_COUNT];
    _renderer.encoders = [NSMutableArray arrayWithCapacity:BUFFER_COUNT];
    _renderer.renderPasses = [NSMutableArray arrayWithCapacity:BUFFER_COUNT];
    
    _renderer.pipelines = [NSMutableArray arrayWithCapacity:20];
    _renderer.pipeDescs = [NSMutableArray arrayWithCapacity:20];
    _renderer.constantBuffers = [NSMutableArray arrayWithCapacity:40];
    _renderer.indexBuffers = [NSMutableArray arrayWithCapacity:40];
    
    _renderer.textures = [NSMutableDictionary dictionaryWithCapacity:250];
    _renderer.vertBuffers = [NSMutableDictionary dictionaryWithCapacity:200];
    
    _renderer.depthStates = [NSMutableArray arrayWithCapacity:16];
    _renderer.samplers = [NSMutableDictionary dictionaryWithCapacity:10];
    _renderer.blendPipelines = [NSMutableDictionary dictionaryWithCapacity:20];
    
    [_renderer defaultPipelines];

    _Mview.delegate = _renderer;
}

@end

