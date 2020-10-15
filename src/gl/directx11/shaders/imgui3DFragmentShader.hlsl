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
  float2 depthInfo : TEXCOORD1;
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

float CalculateDepth(float2 depthInfo)
{
  if (depthInfo.y != 0.0) // orthographic
    return (depthInfo.x / depthInfo.y);
	
  // log-z (perspective)
  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  return log2(depthInfo.x) * halfFcoef;
}

sampler TextureSampler;
Texture2D TextureTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  output.Color0 = input.col * TextureTexture.Sample(TextureSampler, input.uv);
  
  float depth = CalculateDepth(input.depthInfo);
  
  output.Normal = packNormal(float3(0, 0, 0), input.objectInfo.x, depth);
  output.Normal.w = 1.0; // force alpha-blend value
   
  return output;
}
