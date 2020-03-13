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
  float3 normal : NORMAL;
  //float4 colour : COLOR0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
  float3 normal : NORMAL;
  float4 colour : COLOR0;
  float2 fLogDepth : TEXCOORD1;
};

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_worldViewProjectionMatrix;
  float4x4 u_worldMatrix;
  float4 u_colour;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  // making the assumption that the model matrix won't contain non-uniform scale
  float3 worldNormal = normalize(mul(u_worldMatrix, float4(input.normal, 0.0)).xyz);

  output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
  output.uv = input.uv;
  output.normal = worldNormal;
  output.colour = u_colour;// * input.colour;
  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
