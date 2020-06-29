cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

sampler TextureSampler;
Texture2D TextureTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  output.Color0 = input.col * TextureTexture.Sample(TextureSampler, input.uv);
  
  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  float depth = log2(input.fLogDepth.x) * halfFcoef;
  
  output.Normal = packNormal(float3(0, 0, 0), input.objectInfo.x, depth);
  output.Normal.w = 1.0; // force alpha-blend value
   
  return output;
}
