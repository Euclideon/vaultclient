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
  float4 ribbonInfo : COLOR0; // xyz: expand vector; z: pair id (0 or 1)
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
};

cbuffer u_EveryFrame : register(b0)
{
  float4 u_bottomColour;
  float4 u_topColour;

  float u_orientation;
  float u_width;
  float u_textureRepeatScale;
  float u_textureScrollSpeed;
  float u_time;

  float3 _padding;
};

cbuffer u_EveryObject : register(b1)
{
  float4x4 u_worldViewProjectionMatrix;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  // fence horizontal UV pos packed into Y channel
  output.uv = float2(lerp(input.uv.y, input.uv.x, u_orientation) * u_textureRepeatScale - u_time * u_textureScrollSpeed, input.ribbonInfo.w);
  output.colour = lerp(u_bottomColour, u_topColour, input.ribbonInfo.w);

  float3 worldPosition = input.pos + lerp(float3(0, 0, input.ribbonInfo.w) * u_width, input.ribbonInfo.xyz, u_orientation);
  output.pos = mul(u_worldViewProjectionMatrix, float4(worldPosition, 1.0));
  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
