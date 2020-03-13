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
  float2 uv : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = float4(input.pos.xy, 0.f, 1.f);
  output.uv = input.uv;
  return output;
}
