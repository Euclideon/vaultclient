// dear imgui: Renderer for Metal
// This needs to be used along with a Platform Binding (e.g. OSX)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-02-11: Metal: Projecting clipping rectangles correctly using draw_data->FramebufferScale to allow multi-viewports for retina display.
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-07-05: Metal: Added new Metal backend implementation.

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_sdl.h"

#import <Metal/Metal.h>
//#import <QuartzCore/CAMetalLayer.h> // Not suported in XCode 9.2. Maybe a macro to detect the SDK version can be used (something like #if MACOS_SDK >= 10.13 ...)

#include "gl/vcFramebuffer.h"
#include "gl/metal/vcMetal.h"
#include "gl/metal/vcRenderer.h"
#include "udPlatform/udPlatform.h"

#pragma mark - Support classes

// A wrapper around a MTLBuffer object that knows the last time it was reused
@interface MetalBuffer : NSObject
@property (nonatomic, strong) id<MTLBuffer> buffer;
@property (nonatomic, assign) NSTimeInterval lastReuseTime;
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer;
@end

// A singleton that stores long-lived objects that are needed by the Metal
// renderer backend. Stores the render pipeline state cache and the default
// font texture, and manages the reusable buffer cache.
@interface MetalContext : NSObject

@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, nullable) vcTexture *fontTexture;
@property (nonatomic, strong) NSMutableArray<MetalBuffer *> *bufferCache;
@property (nonatomic, assign) NSTimeInterval lastBufferCachePurge;
@property (nonatomic, strong) id<MTLLibrary> library;
@property (nonatomic, strong) MTLRenderPassDescriptor* renderPassDescriptor;
@property (nonatomic, strong) id<MTLRenderPipelineState> renderPipeState;
- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device;
- (void)makeFontTextureWithDevice:(id<MTLDevice>)device;
- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device;
- (void)enqueueReusableBuffer:(MetalBuffer *)buffer;
- (id<MTLRenderPipelineState>)getPipeline:(id<MTLDevice>)device;
- (void)renderDrawData:(ImDrawData *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder;
@end

static MetalContext *g_sharedMetalContext = nil;

#pragma mark - ImGui API implementation

bool ImGui_ImplMetal_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_metal";
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        g_sharedMetalContext = [[MetalContext alloc] init];
    });
    
    g_sharedMetalContext.library = _library;
    ImGui_ImplMetal_CreateDeviceObjects();
    
    return true;
}

void ImGui_ImplMetal_Shutdown()
{
    ImGui_ImplMetal_DestroyDeviceObjects();
}

void ImGui_ImplMetal_NewFrame(SDL_Window *pWindow)
{
    IM_ASSERT(g_sharedMetalContext != nil && "No Metal context. Did you call ImGui_ImplMetal_Init?");
    
    g_sharedMetalContext.renderPassDescriptor = _viewCon.renderer.renderPasses[0];
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Using this causes havoc as you resize the window when imgui sets the scissor rect below
    //io.DisplaySize = ImVec2((float)(_viewCon.Mview.window.frame.size.width), (float)(_viewCon.Mview.window.frame.size.height));
    io.DisplaySize = ImVec2((float)(_viewCon.Mview.drawableSize.width), (float)(_viewCon.Mview.drawableSize.height));
    io.DisplayFramebufferScale = ImVec2(io.DisplaySize.x > 0 ? ((float)(_viewCon.Mview.drawableSize.width) / io.DisplaySize.x) : 0,
                                        io.DisplaySize.y > 0 ? ((float)(_viewCon.Mview.drawableSize.height) / io.DisplaySize.y) : 0);
    
    ImGui_ImplSDL2_NewFrame(pWindow);
    ImGui::NewFrame();

    io.MousePos.x -= [_viewCon.Mview.window contentLayoutRect].origin.x;
    io.MousePos.y -= [_viewCon.Mview.window contentLayoutRect].origin.y;
}

// Metal Render function.
void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data)
{
    [g_sharedMetalContext renderDrawData:draw_data commandBuffer:_viewCon.renderer.commandBuffers[0] commandEncoder:_viewCon.renderer.encoders[0]];
}

bool ImGui_ImplMetal_CreateFontsTexture()
{
    [g_sharedMetalContext makeFontTextureWithDevice:_device];
    
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->TexID = (void *)g_sharedMetalContext.fontTexture; // ImTextureID == void*
    
    return (g_sharedMetalContext.fontTexture != nil);
}

void ImGui_ImplMetal_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    g_sharedMetalContext.fontTexture = nil;
    io.Fonts->TexID = nullptr;
}

bool ImGui_ImplMetal_CreateDeviceObjects()
{
    [g_sharedMetalContext makeDeviceObjectsWithDevice:_device];
    
    ImGui_ImplMetal_CreateFontsTexture();
    
    return true;
}

void ImGui_ImplMetal_DestroyDeviceObjects()
{
    ImGui_ImplMetal_DestroyFontsTexture();
}

#pragma mark - MetalBuffer implementation

@implementation MetalBuffer
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer
{
    if ((self = [super init]))
    {
        _buffer = buffer;
        _lastReuseTime = [NSDate date].timeIntervalSince1970;
    }
    return self;
}
@end

#pragma mark - MetalContext implementation

@implementation MetalContext
- (instancetype)init {
    if ((self = [super init]))
    {
        _bufferCache = [NSMutableArray array];
        _lastBufferCachePurge = [NSDate date].timeIntervalSince1970;
    }
    return self;
}

- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device
{
    MTLDepthStencilDescriptor *depthStencilDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    depthStencilDescriptor.depthWriteEnabled = NO;
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
    self.depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
    g_sharedMetalContext.renderPipeState = [self getPipeline:device];
}

// We are retrieving and uploading the font atlas as a 4-channels RGBA texture here.
// In theory we could call GetTexDataAsAlpha8() and upload a 1-channel texture to save on memory access bandwidth.
// However, using a shader designed for 1-channel texture would make it less obvious to use the ImTextureID facility to render users own textures.
// You can make that change in your implementation.
- (void)makeFontTextureWithDevice:(id<MTLDevice>)device
{
    udUnused(device);
    ImGuiIO &io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    vcTexture *fontText;
    vcTexture_Create(&fontText, width, height, pixels, vcTextureFormat_RGBA8, vcTFM_Nearest, false, vcTWM_Clamp);
    self.fontTexture = fontText;
}

- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device
{
    NSTimeInterval now = [NSDate date].timeIntervalSince1970;
    
    // Purge old buffers that haven't been useful for a while
    if (now - self.lastBufferCachePurge > 1.0)
    {
        NSMutableArray *survivors = [NSMutableArray array];
        for (MetalBuffer *candidate in self.bufferCache)
        {
            if (candidate.lastReuseTime > self.lastBufferCachePurge)
            {
                [survivors addObject:candidate];
            }
        }
        self.bufferCache = [survivors mutableCopy];
        self.lastBufferCachePurge = now;
    }
    
    // See if we have a buffer we can reuse
    MetalBuffer *bestCandidate = nil;
    for (MetalBuffer *candidate in self.bufferCache)
        if (candidate.buffer.length >= length && (bestCandidate == nil || bestCandidate.lastReuseTime > candidate.lastReuseTime))
            bestCandidate = candidate;
    
    if (bestCandidate != nil)
    {
        [self.bufferCache removeObject:bestCandidate];
        bestCandidate.lastReuseTime = now;
        return bestCandidate;
    }
    
    // No luck; make a new buffer
    id<MTLBuffer> backing = [device newBufferWithLength:length options:MTLResourceStorageModeShared];
    return [[MetalBuffer alloc] initWithBuffer:backing];
}

- (void)enqueueReusableBuffer:(MetalBuffer *)buffer
{
    [self.bufferCache addObject:buffer];
}

- (nullable id<MTLRenderPipelineState>)getPipeline:(nonnull id<MTLDevice>)device
{
    NSError *error = nil;
    
    id<MTLFunction> vertexFunction = [_library newFunctionWithName:@"imguiVertexShader"];
    id<MTLFunction> fragmentFunction = [_library newFunctionWithName:@"imguiFragmentShader"];
    
    if (vertexFunction == nil || fragmentFunction == nil)
    {
        NSLog(@"Error: failed to find Metal shader functions in library: %@", error);
        return nil;
    }
    
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    vertexDescriptor.attributes[0].offset = IM_OFFSETOF(ImDrawVert, pos);
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].offset = IM_OFFSETOF(ImDrawVert, uv);
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.attributes[2].offset = IM_OFFSETOF(ImDrawVert, col);
    vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4; // color
    vertexDescriptor.attributes[2].bufferIndex = 0;
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);
    
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
    pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
#else
# error "Unsupported platform!"
#endif
    
    id<MTLRenderPipelineState> renderPipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (error != nil)
    {
        NSLog(@"Error: failed to create Metal pipeline state: %@", error);
    }
    
    return renderPipelineState;
}

- (void)renderDrawData:(ImDrawData *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || drawData->CmdListsCount == 0)
        return;
    
    [commandEncoder setCullMode:MTLCullModeNone];
    [commandEncoder setDepthStencilState:g_sharedMetalContext.depthStencilState];
    
    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    MTLViewport viewport =
    {
        .originX = 0.0,
        .originY = 0.0,
        .width = double(fb_width),
        .height = double(fb_height),
        .znear = 0.0,
        .zfar = 1.0
    };
    [commandEncoder setViewport:viewport];
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float N = viewport.znear;
    float F = viewport.zfar;
    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,           0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),     0.0f,   0.0f },
        { 0.0f,         0.0f,        1/(F-N),   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T), N/(F-N),   1.0f },
    };
    
    [commandEncoder setVertexBytes:&ortho_projection length:sizeof(ortho_projection) atIndex:1];
    
    size_t vertexBufferLength = drawData->TotalVtxCount * sizeof(ImDrawVert);
    size_t indexBufferLength = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    MetalBuffer* vertexBuffer = [self dequeueReusableBufferOfLength:vertexBufferLength device:commandBuffer.device];
    MetalBuffer* indexBuffer = [self dequeueReusableBufferOfLength:indexBufferLength device:commandBuffer.device];
    
    [commandEncoder setRenderPipelineState:g_sharedMetalContext.renderPipeState];
    
    [commandEncoder setVertexBuffer:vertexBuffer.buffer offset:0 atIndex:0];
    
    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
    
    // Render command lists
    size_t vertexBufferOffset = 0;
    size_t indexBufferOffset = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        ImDrawIdx idx_buffer_offset = 0;
        
        memcpy((char *)vertexBuffer.buffer.contents + vertexBufferOffset, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy((char *)indexBuffer.buffer.contents + indexBufferOffset, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        
        [commandEncoder setVertexBufferOffset:vertexBufferOffset atIndex:0];
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback (registered via ImDrawList::AddCallback)
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = udMin(((pcmd->ClipRect.z - clip_off.x) * clip_scale.x), (float)g_sharedMetalContext.renderPassDescriptor.colorAttachments[0].texture.width);
                clip_rect.w = udMin(((pcmd->ClipRect.w - clip_off.y) * clip_scale.y), (float)g_sharedMetalContext.renderPassDescriptor.colorAttachments[0].texture.height);
                
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z > 1.0f && clip_rect.w > 1.0f)
                {
                    // Apply scissor/clipping rectangle
                    MTLScissorRect scissorRect =
                    {
                        .x = NSUInteger(clip_rect.x),
                        .y = NSUInteger(clip_rect.y),
                        .width = NSUInteger(clip_rect.z - clip_rect.x),
                        .height = NSUInteger(clip_rect.w - clip_rect.y)
                    };
                    [commandEncoder setScissorRect:scissorRect];
                    
                    // Bind texture, Draw
                    if (pcmd->TextureId != NULL)
                    {
                        vcShader_BindTexture(nullptr, (vcTexture *)pcmd->TextureId, 0);
                    }
                    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                               indexCount:pcmd->ElemCount
                                                indexType:sizeof(ImDrawIdx) == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
                                              indexBuffer:indexBuffer.buffer
                                        indexBufferOffset:indexBufferOffset + idx_buffer_offset];
                }
            }
            idx_buffer_offset += pcmd->ElemCount * sizeof(ImDrawIdx);
        }
        
        vertexBufferOffset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        indexBufferOffset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }
    
    id weakSelf = self;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer>)
     {
         dispatch_async(dispatch_get_main_queue(), ^{
             [weakSelf enqueueReusableBuffer:vertexBuffer];
             [weakSelf enqueueReusableBuffer:indexBuffer];
         });
     }];
}

@end
