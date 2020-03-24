#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct PCU
{
  float4 v_position [[position]];
  float4 v_color;
  float2 v_uv;
};

fragment float4 main0(PCU in [[stage_in]], texture2d<float, access::sample> FFSimg [[texture(0)]], sampler FFSsampler [[sampler(0)]])
{
  float4 texCol = FFSimg.sample(FFSsampler, in.v_uv);
  return float4(texCol.xyz * in.v_color.xyz, texCol.w * in.v_color.w);
}
