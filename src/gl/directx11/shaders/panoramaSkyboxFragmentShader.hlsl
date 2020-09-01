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
  float4 clip : TEXCOORD0;
};

cbuffer u_EveryFrame : register(b0)
{
  float4x4 u_inverseViewProjection;
};

sampler albedoSampler;
Texture2D albedoTexture;

#define PI 3.14159265359

float2 directionToLatLong(float3 dir)
{
  float2 longlat = float2(atan2(dir.x, dir.y) + PI, acos(dir.z));
  return longlat / float2(2.0 * PI, PI);
}

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  
  // work out 3D point
  float4 point3D = mul(u_inverseViewProjection, float4(input.clip.xy, 1.0, 1.0));
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  output.Color0 = albedoTexture.Sample(albedoSampler, directionToLatLong(point3D.xyz));
  
  output.Normal = float4(0, 0, 0, 0);
  return output;
}
