#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct BlVSInput
{
  float3 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
};

struct BlVSOutput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float2 uv2;
};

struct BlVSUniforms
{
  float4 u_stepSize; // remember: requires 16 byte alignment
};

vertex BlVSOutput
main0(BlVSInput in [[stage_in]], constant BlVSUniforms& uniforms [[buffer(1)]])
{
  BlVSOutput out;
  
  out.pos = float4(in.pos.x, in.pos.y, 0.0, 1.0);
  
  // sample on edges, taking advantage of bilinear sampling
  float2 sampleOffset = 1.42 * uniforms.u_stepSize.xy;
  float2 uv = float2(in.uv.x, 1.0 - in.uv.y);
  out.uv0 = uv - sampleOffset;
  out.uv1 = uv;
  out.uv2 = uv + sampleOffset;
  
  return out;
}
