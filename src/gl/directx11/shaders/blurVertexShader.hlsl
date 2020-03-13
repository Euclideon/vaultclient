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
};

cbuffer u_EveryFrame : register(b0)
{
  float4 u_stepSize; // remember: requires 16 byte alignment
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);

  // sample on edges, taking advantage of bilinear sampling
  float2 sampleOffset = 1.42 * u_stepSize.xy;
  float2 uv = float2(input.uv.x, 1.0 - input.uv.y);
  output.uv0 = uv - sampleOffset;
  output.uv1 = uv;
  output.uv2 = uv + sampleOffset;

  return output;
}
