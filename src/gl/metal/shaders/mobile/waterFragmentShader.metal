#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct WVSOutput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float4 fragEyePos;
  float3 color;
};

// g_WaterFragmentShader
struct WFSUniforms
{
  float4 u_specularDir;
  float4x4 u_eyeNormalMatrix;
  float4x4 u_inverseViewMatrix;
};

float2 directionToLatLong(float3 dir)
{
  float2 longlat = float2(atan2(dir.x, dir.y) + M_PI_F, acos(dir.z));
  return longlat / float2(2.0 * M_PI_F, M_PI_F);
}

fragment float4
main0(WVSOutput in [[stage_in]], constant WFSUniforms& uWFS [[buffer(2)]], texture2d<float, access::sample> WFSNormal [[texture(0)]], sampler WFSNsampler [[sampler(0)]], texture2d<float, access::sample> WFSSkybox [[texture(1)]], sampler WFSSBsampler [[sampler(1)]])
{
  float3 specularDir = normalize(uWFS.u_specularDir.xyz);
  
  float3 normal0 = WFSNormal.sample(WFSNsampler, in.uv0).xyz * float3(2.0) - float3(1.0);
  float3 normal1 = WFSNormal.sample(WFSNsampler, in.uv1).xyz * float3(2.0) - float3(1.0);
  float3 normal = normalize((normal0.xyz * normal1.xyz));
  
  float3 eyeToFrag = normalize(in.fragEyePos.xyz);
  float3 eyeSpecularDir = normalize((uWFS.u_eyeNormalMatrix * float4(specularDir, 0.0)).xyz);
  float3 eyeNormal = normalize((uWFS.u_eyeNormalMatrix * float4(normal, 0.0)).xyz);
  float3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));
  
  float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
  float nDotL = -dot(eyeNormal, eyeToFrag);
  float fresnel = nDotL * 0.5 + 0.5;
  
  float specular = pow(nDotS, 250.0) * 0.5;
  
  float3 deepFactor = float3(0.35, 0.35, 0.35);
  float3 shallowFactor = float3(1.0, 1.0, 0.7);
  
  float distanceToShore = 1.0; // maybe TODO
  float3 refractionColor = in.color.xyz * mix(shallowFactor, deepFactor, distanceToShore);
  
  // reflection
  float4 worldFragPos = uWFS.u_inverseViewMatrix * float4(eyeReflectionDir, 0.0);
  float4 skybox = WFSSkybox.sample(WFSSBsampler, directionToLatLong(normalize(worldFragPos.xyz)));
  float3 reflectionColor = skybox.xyz;
  
  float3 finalColor = mix(reflectionColor, refractionColor, fresnel * 0.75) + float3(specular);
  return float4(finalColor, 1.0);
}
