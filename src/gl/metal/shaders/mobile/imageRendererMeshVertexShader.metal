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

struct PNUVVSInput
{
  float3 pos [[attribute(0)]];
  float3 normal [[attribute(1)]]; // unused
  float2 uv [[attribute(2)]];
};

struct IRMVSUniforms
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_color;
  float4 u_screenSize; // unused
};

vertex PUC
main0(PNUVVSInput in [[stage_in]], constant IRMVSUniforms& uniforms [[buffer(1)]])
{
  PUC out;
  
  out.pos = uniforms.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
  out.uv = float2(in.uv[0], 1.0 - in.uv[1]);
  out.color = uniforms.u_color;
  
  return out;
}
