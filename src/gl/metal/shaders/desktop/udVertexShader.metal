#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct UDVSInput
{
  float3 a_position [[attribute(0)]];
  float2 a_texCoord [[attribute(1)]];
};

struct UDVSOutput
{
  float4 v_position [[position]];
  float2 uv;
};

vertex UDVSOutput
main0(UDVSInput in [[stage_in]])
{
  UDVSOutput out;
  out.v_position = float4(in.a_position.xy, 0.0, 1.0);
  out.uv = in.a_texCoord;
  return out;
}
