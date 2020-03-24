#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct FCFSInput
{
  float4 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
  float3 normal [[attribute(2)]];
  float4 color [[attribute(3)]];
};

float4 main0(FCFSInput in [[stage_in]])
{
  return float4(in.color);
}
