#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct DOFSInput
{
  float4 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
  float3 normal [[attribute(2)]];
  float4 color [[attribute(3)]];
};

float4 main0(DOFSInput in [[stage_in]])
{
  return float4(0.0, 0.0, 0.0, 0.0);
}
