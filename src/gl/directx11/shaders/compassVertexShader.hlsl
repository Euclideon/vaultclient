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
  float3 normal : NORMAL;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float3 normal : COLOR0;
  float4 colour : COLOR1;
  float3 sunDirection : COLOR2;
  float4 fragClipPosition : COLOR3;
  float2 fLogDepth : TEXCOORD0;
};

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_colour;
  float3 u_sunDirection;
  float _padding;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
  output.normal = (input.normal * 0.5) + 0.5;
  output.colour = u_colour;
  output.sunDirection = u_sunDirection;
  output.fragClipPosition = output.pos;

  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
