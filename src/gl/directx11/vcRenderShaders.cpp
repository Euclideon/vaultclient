#include "gl/vcRenderShaders.h"
#include "udPlatformUtil.h"

#define FRAG_HEADER "cbuffer u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };"
#define VERT_HEADER "cbuffer u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };"

const char *const g_udGPURenderQuadVertexShader = VERT_HEADER R"shader(
  struct VS_INPUT
  {
    float4 pos : POSITION;
    float4 colour  : COLOR0;
    float2 corner: TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.colour = input.colour.bgra;

    // Points
    float4 off = float4(input.pos.www * 2.0, 0);
    float4 pos0 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.www, 1.0));
    float4 pos1 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xww, 1.0));
    float4 pos2 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyw, 1.0));
    float4 pos3 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyw, 1.0));
    float4 pos4 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wwz, 1.0));
    float4 pos5 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xwz, 1.0));
    float4 pos6 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyz, 1.0));
    float4 pos7 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyz, 1.0));

    float4 minPos, maxPos;
    minPos = min(pos0, pos1);
    minPos = min(minPos, pos2);
    minPos = min(minPos, pos3);
    minPos = min(minPos, pos4);
    minPos = min(minPos, pos5);
    minPos = min(minPos, pos6);
    minPos = min(minPos, pos7);
    maxPos = max(pos0, pos1);
    maxPos = max(maxPos, pos2);
    maxPos = max(maxPos, pos3);
    maxPos = max(maxPos, pos4);
    maxPos = max(maxPos, pos5);
    maxPos = max(maxPos, pos6);
    maxPos = max(maxPos, pos7);
    output.pos = minPos + (maxPos - minPos) * 0.5;

    float2 pointSize = float2(maxPos.x - minPos.x, maxPos.y - minPos.y);
    output.pos.xy += pointSize * input.corner * float2(0.5, 0.5);
    return output;
  }
)shader";

const char *const g_udGPURenderQuadFragmentShader = FRAG_HEADER R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  float4 main(PS_INPUT input) : SV_Target
  {
    return input.colour;
  }
)shader";
