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

sampler sampler0;
sampler sampler1;
Texture2D u_normalMap;
Texture2D u_skybox;

float4 main(PS_INPUT input) : SV_Target
{
  float3 specularDir = normalize(u_specularDir.xyz);

  float3 normal0 = u_normalMap.Sample(sampler0, input.uv0).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
  float3 normal1 = u_normalMap.Sample(sampler0, input.uv1).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
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
  float4 skybox = u_skybox.Sample(sampler1, directionToLatLong(normalize(worldFragPos.xyz)));
  float3 reflectionColour = skybox.xyz;

  float3 finalColour = lerp(reflectionColour, refractionColour, fresnel * 0.75) + float3(specular, specular, specular);
  return float4(finalColour, 1.0);
}
