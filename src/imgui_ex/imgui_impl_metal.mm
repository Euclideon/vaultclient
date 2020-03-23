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
#include "imgui_impl_gl.h"
#include "imgui_impl_sdl.h"

#import <Metal/Metal.h>
//#import <QuartzCore/CAMetalLayer.h> // Not suported in XCode 9.2. Maybe a macro to detect the SDK version can be used (something like #if MACOS_SDK >= 10.13 ...)

#include "gl/vcFramebuffer.h"
#include "gl/metal/vcMetal.h"
#include "gl/metal/vcRenderer.h"
#include "udPlatform.h"

#pragma mark - Support classes

//GL Data
vcShader *pMetalShader = nullptr;
vcMesh *pMetalMesh = nullptr;
vcShaderSampler *pMetalSampler = nullptr;
static vcShaderConstantBuffer *metalMatrix = nullptr;

// A singleton that stores long-lived objects that are needed by the Metal
// renderer backend. Stores the render pipeline state cache and the default
// font texture, and manages the reusable buffer cache.
@interface MetalContext : NSObject

@property (nonatomic, strong) id<MTLRenderPipelineState> pipeline;
- (void)makeDeviceObjects;
- (void)renderDrawData:(ImDrawData *)drawData;
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

    ImGui_ImplMetal_CreateDeviceObjects();

    return true;
}

void ImGui_ImplMetal_Shutdown()
{
    ImGui_ImplMetal_DestroyDeviceObjects();
}

void ImGui_ImplMetal_NewFrame(SDL_Window *pWindow)
{
    if (!ImGui::GetIO().Fonts->IsBuilt())
        ImGuiGL_CreateFontsTexture();
    
    ImGui_ImplSDL2_NewFrame(pWindow);
    ImGui::NewFrame();
}

// Metal Render function.
void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data)
{
    [g_sharedMetalContext renderDrawData:draw_data];
}

bool ImGui_ImplMetal_CreateDeviceObjects()
{
    [g_sharedMetalContext makeDeviceObjects];

    ImGuiGL_CreateFontsTexture();

    return true;
}

void ImGui_ImplMetal_DestroyDeviceObjects()
{
    ImGuiGL_DestroyFontsTexture();
}

#pragma mark - MetalContext implementation

@implementation MetalContext
- (instancetype)init {
    self = [super init];
    return self;
}

- (void)makeDeviceObjects
{
    vcShader_CreateFromFile(&pMetalShader, "asset://assets/shaders/imguiVertexShader", "asset://assets/shaders/imguiFragmentShader", vcImGuiVertexLayout);
    vcShader_Bind(pMetalShader);
    vcShader_GetConstantBuffer(&metalMatrix, pMetalShader, "u_EveryFrame", sizeof(udFloat4x4));
    _pipeline = [self getPipeline:_device];
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
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].offset = 8;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.attributes[2].offset = 16;
    vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4Normalized; // color
    vertexDescriptor.attributes[2].bufferIndex = 0;
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = 20;

    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float;
#elif UDPLATFORM_OSX
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
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
{
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || drawData->CmdLists == nullptr)
        return;

    drawData->ScaleClipRects(io.DisplayFramebufferScale);
    vcGLState_SetBlendMode(vcGLSBM_Interpolative, true);
    vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);

    vcGLState_SetDepthStencilMode(vcGLSDM_Always, false);

    vcGLState_SetViewport(0, 0, fb_width, fb_height);
    const udFloat4x4 ortho_projection = udFloat4x4::create(
       2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f,
       0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       -1.0f, 1.0f, 0.0f, 1.0f
    );

    vcShader_Bind(pMetalShader);
    [_renderer.encoders[0] setRenderPipelineState:_pipeline];

    vcShader_BindConstantBuffer(pMetalShader, metalMatrix, &ortho_projection, sizeof(ortho_projection));
    vcShader_GetSamplerIndex(&pMetalSampler, pMetalShader, "Texture");

    if (drawData->CmdListsCount != 0 && pMetalMesh == nullptr)
        vcMesh_Create(&pMetalMesh, vcImGuiVertexLayout, 3, drawData->CmdLists[0]->VtxBuffer.Data, (uint32_t)drawData->CmdLists[0]->VtxBuffer.Size, drawData->CmdLists[0]->IdxBuffer.Data, (uint32_t)drawData->CmdLists[0]->IdxBuffer.Size, vcMF_Dynamic);

    // Draw
    ImVec2 pos = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        uint32_t totalDrawn = 0;

        vcMesh_UploadData(pMetalMesh, vcImGuiVertexLayout, 3, (void*)cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size, (void*)cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                vcGLState_Scissor((int)(pcmd->ClipRect.x - pos.x), (int)(pcmd->ClipRect.y - pos.y), (int)(pcmd->ClipRect.z - pos.x), (int)(pcmd->ClipRect.w - pos.y));
                vcShader_BindTexture(pMetalShader, (vcTexture*)pcmd->TextureId, 0, pMetalSampler);
                vcMesh_Render(pMetalMesh, pcmd->ElemCount / 3, totalDrawn / 3);
            }
            totalDrawn += pcmd->ElemCount;
        }
    }
}

@end
