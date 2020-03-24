#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct BlVSOutput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float2 uv2;
};

constant static float4 multipliers[3] = {float4(0.0, 0.0, 0.0, 0.27901), float4(1.0, 1.0, 1.0, 0.44198), float4(0.0, 0.0, 0.0, 0.27901)};

fragment float4
main0(BlVSOutput in [[stage_in]], texture2d<float, access::sample> BlFSimg [[texture(0)]], sampler BlFSSampler [[sampler(0)]])
{
  float4 color = float4(0.0, 0.0, 0.0, 0.0);
  
  color += multipliers[0] * BlFSimg.sample(BlFSSampler, in.uv0);
  color += multipliers[1] * BlFSimg.sample(BlFSSampler, in.uv1);
  color += multipliers[2] * BlFSimg.sample(BlFSSampler, in.uv2);
  
  return color;
}
