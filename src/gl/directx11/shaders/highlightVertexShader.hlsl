cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

struct VS_INPUT
{
  float3 pos : POSITION;
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv0  : TEXCOORD0;
  float2 uv1  : TEXCOORD1;
  float2 uv2  : TEXCOORD2;
  float2 uv3  : TEXCOORD3;
  float2 uv4  : TEXCOORD4;
  float4 colour : COLOR0;
  float4 stepSizeThickness : COLOR1;
};

static float2 searchKernel[4] = { float2(-1, -1), float2(1, -1), float2(-1,  1), float2(1,  1) };

cbuffer u_EveryFrame : register(b0)
{
  float4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
  float4 u_colour;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);
  output.colour = u_colour;
  output.stepSizeThickness = u_stepSizeThickness;

  output.uv0 = input.uv;
  output.uv1 = input.uv + u_stepSizeThickness.xy * searchKernel[0];
  output.uv2 = input.uv + u_stepSizeThickness.xy * searchKernel[1];
  output.uv3 = input.uv + u_stepSizeThickness.xy * searchKernel[2];
  output.uv4 = input.uv + u_stepSizeThickness.xy * searchKernel[3];

  return output;
}
