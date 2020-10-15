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
  float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv0  : TEXCOORD0;
  float2 uv1  : TEXCOORD1;
  float4 fragEyePos : COLOR0;
  float3 colour : COLOR1;
  float2 depthInfo : TEXCOORD2;
};

cbuffer u_EveryFrameVert : register(b0)
{
  float4 u_time;
};

cbuffer u_EveryObject : register(b1)
{
  float4 u_colourAndSize;
  float4x4 u_worldViewMatrix;
  float4x4 u_worldViewProjectionMatrix;
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

  float uvScaleBodySize = u_colourAndSize.w; // packed here

  // scale the uvs with time
  float uvOffset = u_time.x * 0.0625;
  output.uv0 = uvScaleBodySize * input.uv.xy * float2(0.25, 0.25) - float2(uvOffset, uvOffset);
  output.uv1 = uvScaleBodySize * input.uv.yx * float2(0.50, 0.50) - float2(uvOffset, uvOffset * 0.75);

  output.fragEyePos = mul(u_worldViewMatrix, float4(input.pos, 1.0));
  output.colour = u_colourAndSize.xyz;
  output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));

  output.depthInfo = PackDepth(output.pos);
  
  return output;
}
