#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct CVSInput
{
  float3 a_pos [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
};

struct CVSOutput
{
  float4 pos [[position]];
  float4 v_color;
  float3 v_normal;
  float4 v_fragClipPosition;
  float3 v_sunDirection;
};

struct CVSUniforms
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_color;
  float3 u_sunDirection;
};

vertex CVSOutput
main0(CVSInput in [[stage_in]], constant CVSUniforms& uCVS [[buffer(1)]])
{
  CVSOutput out;
  out.v_fragClipPosition = uCVS.u_worldViewProjectionMatrix * float4(in.a_pos, 1.0);
  out.pos = out.v_fragClipPosition;
  out.v_normal = (in.a_normal * 0.5) + 0.5;
  out.v_color = uCVS.u_color;
  out.v_sunDirection = uCVS.u_sunDirection;
  return out;
}
