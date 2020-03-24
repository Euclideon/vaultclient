#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct PUC
{
  float4 pos [[position]];
  float2 uv;
  float4 color;
};

fragment float4
main0(PUC in [[stage_in]], texture2d<float, access::sample> IRFSimg [[texture(0)]], sampler IRFSsampler [[sampler(0)]])
{
  float4 col = IRFSimg.sample(IRFSsampler, in.uv);
  return col * in.color;
}
