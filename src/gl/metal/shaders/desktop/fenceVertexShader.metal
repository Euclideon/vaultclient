#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct FVSUniforms
{
  float4 u_bottomColor;
  float4 u_topColor;

  float u_orientation;
  float u_width;
  float u_textureRepeatScale;
  float u_textureScrollSpeed;
  float u_time;
};

struct FVSEveryObject
{
  float4x4 u_worldViewProjectionMatrix;
};

struct FVSInput
{
  float3 a_position [[attribute(0)]];
  float2 a_uv [[attribute(1)]];
  float4 a_ribbonInfo [[attribute(2)]]; // xyz: expand floattor; z: pair id (0 or 1)
};

struct PCU
{
  float4 v_position [[position]];
  float4 v_color;
  float2 v_uv;
};

vertex PCU
main0(FVSInput in [[stage_in]], constant FVSUniforms& uFVS [[buffer(1)]], constant FVSEveryObject& uFVSEO [[buffer(2)]])
{
  PCU out;

  // fence horizontal UV pos packed into Y channel
  out.v_uv = float2(mix(in.a_uv.y, in.a_uv.x, uFVS.u_orientation) * uFVS.u_textureRepeatScale - uFVS.u_time * uFVS.u_textureScrollSpeed, in.a_ribbonInfo.w);
  out.v_color = mix(uFVS.u_bottomColor, uFVS.u_topColor, in.a_ribbonInfo.w);

  // fence or flat
  float3 worldPosition = in.a_position + mix(float3(0, 0, in.a_ribbonInfo.w) * uFVS.u_width, in.a_ribbonInfo.xyz, uFVS.u_orientation);

  out.v_position = uFVSEO.u_worldViewProjectionMatrix * float4(worldPosition, 1.0);
  return out;
}
