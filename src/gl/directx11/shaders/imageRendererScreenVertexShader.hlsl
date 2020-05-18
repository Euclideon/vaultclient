cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct VS_INPUT
{
  float3 pos : POSITION;
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
  float4 colour : COLOR0;
};

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_colour;
  float4 u_screenSize;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.pos = mul(u_worldViewProjectionMatrix, float4(0, 0, 0, 1.0));
  output.pos.xy += u_screenSize.z * output.pos.w * u_screenSize.xy * input.pos.xy; // expand billboard

  output.uv = input.uv;
  output.colour = u_colour;

  return output;
}
