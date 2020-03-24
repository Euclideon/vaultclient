#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct SVSInput
{
  float3 a_position [[attribute(0)]];
  float2 a_texCoord [[attribute(1)]];
};

struct SVSOutput
{
  float4 position [[position]];
  float2 v_texCoord;
};

vertex SVSOutput
main0(SVSInput in [[stage_in]])
{
  SVSOutput out;
  out.position = float4(in.a_position.xy, 0.0, 1.0);
  out.v_texCoord = float2(in.a_texCoord.x, 1.0 - in.a_texCoord.y);
  return out;
}
