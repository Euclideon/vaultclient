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
  float3 normal : NORMAL; // unused
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
  float4 colour : COLOR0;
  float2 depthInfo : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_colour;
  float4 u_screenSize;
};

float2 PackDepth(float4 clipPos)
{
  if (u_worldViewProjectionMatrix[3][1] == 0.0) // orthographic
    return float2(clipPos.z, clipPos.w);

  // perspective
  return float2(1.0 + clipPos.w, 0);
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
  output.uv = input.uv;
  output.colour = u_colour;
  output.depthInfo = PackDepth(output.pos);
  output.objectInfo.x = u_screenSize.w;

  return output;
}
