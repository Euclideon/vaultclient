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
  float4 tintColour : COLOR0;
};

cbuffer u_EveryFrame : register(b0)
{
  float4 u_tintColour; //0 is full colour, 1 is full image
  float4 u_imageSize; //For purposes of tiling/stretching
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = float4(input.pos.xy, 0.f, 1.f);
  output.uv = float2(input.uv.x, 1.0 - input.uv.y) / u_imageSize.xy;
  output.tintColour = u_tintColour;
  return output;
}
