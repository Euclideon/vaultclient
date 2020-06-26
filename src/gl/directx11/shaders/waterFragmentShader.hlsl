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
  float2 uv0 : TEXCOORD0;
  float2 uv1 : TEXCOORD1;
  float4 fragEyePos : COLOR0;
  float4 colour : COLOR1;
  float2 fLogDepth : TEXCOORD2;
};

cbuffer u_EveryFrameFrag : register(b0)
{
  float4 u_specularDir;
  float4x4 u_eyeNormalMatrix;
  float4x4 u_inverseViewMatrix;
};

#define PI 3.14159265359

float2 directionToLatLong(float3 dir)
{
  float2 longlat = float2(atan2(dir.x, dir.y) + PI, acos(dir.z));
  return longlat / float2(2.0 * PI, PI);
}

sampler normalMapSampler;
Texture2D normalMapTexture;

sampler skyboxSampler;
Texture2D skyboxTexture;

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};


float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  
  float3 specularDir = normalize(u_specularDir.xyz);

  float3 normal0 = normalMapTexture.Sample(normalMapSampler, input.uv0).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
  float3 normal1 = normalMapTexture.Sample(normalMapSampler, input.uv1).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
  float3 normal = normalize((normal0.xyz + normal1.xyz));

  float3 eyeToFrag = normalize(input.fragEyePos.xyz);
  float3 eyeSpecularDir = normalize(mul(u_eyeNormalMatrix, float4(specularDir, 0.0)).xyz);
  float3 eyeNormal = normalize(mul(u_eyeNormalMatrix, float4(normal, 0.0)).xyz);
  float3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));

  float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
  float nDotL = -dot(eyeNormal, eyeToFrag);
  float fresnel = nDotL * 0.5 + 0.5;

  float specular = pow(nDotS, 50.0) * 0.5;

  float3 deepFactor = float3(0.35, 0.35, 0.35);
  float3 shallowFactor = float3(1.0, 1.0, 0.6);

  float waterDepth = pow(max(0.0, dot(normal, float3(0.0, 0.0, 1.0))), 5.0); // guess 'depth' based on normal direction
  float3 refractionColour = input.colour.xyz * lerp(shallowFactor, deepFactor, waterDepth);

  // reflection
  float4 worldFragPos = mul(u_inverseViewMatrix, float4(eyeReflectionDir, 0.0));
  float4 skybox = skyboxTexture.Sample(skyboxSampler, directionToLatLong(normalize(worldFragPos.xyz)));
  float3 reflectionColour = skybox.xyz;

  float3 finalColour = lerp(reflectionColour, refractionColour, fresnel * 0.75) + float3(specular, specular, specular);
  output.Color0 = float4(finalColour, 1.0);
  
  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;
  
  // dull colour until sort out ECEF water
  output.Color0 = output.Color0 * 0.3 + float4(0.2, 0.4, 0.7, 1.0);
  
  output.Normal = packNormal(float3(0.0, 0.0, 0.0), 0.0, output.Depth0);
  return output;
}
