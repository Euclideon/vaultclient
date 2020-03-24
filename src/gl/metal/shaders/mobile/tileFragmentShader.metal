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

fragment float4
main0(PCU in [[stage_in]], texture2d<float, access::sample> TFSimg [[texture(0)]], sampler TFSsampler [[sampler(0)]])
{
  float4 col = TFSimg.sample(TFSsampler, in.v_uv);
  return float4(col.xyz * in.v_color.xyz, in.v_color.w);
};
