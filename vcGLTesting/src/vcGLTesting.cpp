#include "udPlatform.h"

#include "vcFramebuffer.h"
#include "vcGLState.h"
#include "vcMesh.h"
#include "vcShader.h"
#include "vcTexture.h"

#include "SDL2/SDL.h"

enum vcGLTestingStage
{
  vcGLTS_RedBackBuffer,
  vcGLTS_GreenBackBuffer,
  vcGLTS_BlueBackBuffer,
  vcGLTS_NoClearBuffer,

  vcGLTS_ReadPixels,

  vcGLTS_RenderUVScreenQuad,

  vcGLTS_Count,
};

#if GRAPHICS_API_D3D11
const char passthroughVertexShader[] = R"hlsl(
struct VS_INPUT
{
  float3 pos : POSITION;
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = float4(input.pos.xy, 0.f, 1.f);
  output.uv = input.uv;
  return output;
}
)hlsl";

const char passthroughFragmentShader[] = R"hlsl(
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  output.Color0 = float4(input.uv.xy, 0.0, 1.0);
  return output;
}
)hlsl";
#elif GRAPHICS_API_OPENGL
const char passthroughVertexShader[] = R"glsl(
#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
}
)glsl";

const char passthroughFragmentShader[] = R"glsl(
#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_Color;

void main()
{
    out_var_Color = vec4(in_var_TEXCOORD0.xy, 0.0, 1.0);
}
)glsl";
#elif GRAPHICS_API_METAL
const char passthroughVertexShader[] = R"metal(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 TEXCOORD0 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 POSITION [[attribute(0)]];
    float2 TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.POSITION.xy, 0.0, 1.0);
    out.TEXCOORD0 = in.TEXCOORD0;
    return out;
}
)metal";

const char passthroughFragmentShader[] = R"metal(
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 Color [[color(0)]];
};

struct main0_in
{
    float2 TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.Color = float4(in.TEXCOORD0.xy, 0.0, 1.0);
    return out;
}
)metal";
#endif

// screen quad
const vcP3UV2Vertex screenQuadVertices[4]{ { { -1.f, -1.f, 0.f },{ 0, 1 } },{ { 1.f, -1.f, 0.f },{ 1, 1 } },{ { 1.f, 1.f, 0.f },{ 1, 0 } },{ { -1.f, 1.f, 0.f },{ 0, 0 } } };
const uint32_t screenQuadIndices[6] = { 0, 1, 2, 0, 2, 3 };

int main(int argc, char **args)
{
  uint32_t windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_ANDROID
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

#if GRAPHICS_API_OPENGL
  windowFlags |= SDL_WINDOW_OPENGL;
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    exit(1);

  SDL_Window *window = SDL_CreateWindow("vcGLTesting", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, (Uint32)(windowFlags));
  if (window == nullptr)
    exit(2);

  vcFramebuffer *pDefaultFramebuffer = nullptr;
  if (!vcGLState_Init(window, &pDefaultFramebuffer))
    exit(3);

  vcGLState_SetViewport(0, 0, 1280, 720);

  vcGLTestingStage stage = vcGLTS_RedBackBuffer;
  bool running = true;
  while (running)
  {
    vcGLTestingStage prevStage = stage;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        running = false;
      else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
        vcGLState_ResizeBackBuffer(event.window.data1, event.window.data2);
      else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_SPACE)
        stage = (vcGLTestingStage)(stage + 1);
    }

    switch (stage)
    {
    case vcGLTS_RedBackBuffer:
      vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFFFF0000);
      break;
    case vcGLTS_GreenBackBuffer:
      vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFF00FF00);
      break;
    case vcGLTS_BlueBackBuffer:
      vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFF0000FF);
      break;
    case vcGLTS_NoClearBuffer:
      if (prevStage != stage)
        vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFFFFFF00);
      else
        vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_None);
      break;
    case vcGLTS_ReadPixels:
    {
      vcTexture *pTexture = nullptr;
      vcTexture_CreateAdv(&pTexture, vcTextureType_Texture2D, 1280, 720, 1, nullptr, vcTextureFormat_RGBA8, vcTFM_Nearest, false, vcTWM_Repeat, vcTCF_RenderTarget, 0);
      vcFramebuffer *pFramebuffer = nullptr;
      vcFramebuffer_Create(&pFramebuffer, pTexture);
      vcFramebuffer_Bind(pFramebuffer, vcFramebufferClearOperation_All, 0xFFFF00FF);
      uint32_t colour = 0x0;
      vcTexture_BeginReadPixels(pTexture, 0, 0, 1, 1, &colour, pFramebuffer);
      vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, colour);
      vcFramebuffer_Destroy(&pFramebuffer);
      vcTexture_Destroy(&pTexture);
      break;
    }
    case vcGLTS_RenderUVScreenQuad:
    {
      vcShader *pShader = nullptr;
      vcShader_CreateFromText(&pShader, passthroughVertexShader, passthroughFragmentShader, vcP3UV2VertexLayout);
      vcShader_Bind(pShader);
      vcMesh *pMesh = nullptr;
      vcMesh_Create(&pMesh, vcP3UV2VertexLayout, udLengthOf(vcP3UV2VertexLayout), screenQuadVertices, udLengthOf(screenQuadVertices), screenQuadIndices, udLengthOf(screenQuadIndices), vcMF_None);
      vcFramebuffer_Bind(pDefaultFramebuffer, vcFramebufferClearOperation_All, 0xFF000000);
      vcGLState_SetFaceMode(vcGLSFM_Solid, vcGLSCM_None);
      vcShader_Bind(pShader);
      vcMesh_Render(pMesh);
      vcMesh_Destroy(&pMesh);
      vcShader_Bind(nullptr);
      vcShader_DestroyShader(&pShader);
      break;
    }
    case vcGLTS_Count:
      stage = vcGLTS_RedBackBuffer;
      break;
    }

    vcGLState_Present(window);
  }

  vcGLState_Deinit();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
