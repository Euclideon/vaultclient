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

struct IRBVSInput
{
  float3 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
};

struct IRBVSUniforms
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_color;
  float4 u_screenSize;
};

vertex PUC
main0(IRBVSInput in [[stage_in]], constant IRBVSUniforms& uniforms [[buffer(1)]])
{
  PUC out;
  
  out.pos = uniforms.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
  out.pos.xy += uniforms.u_screenSize.z * out.pos.w * uniforms.u_screenSize.xy * float2(in.uv.x * 2.0 - 1.0, in.uv.y * 2.0 - 1.0); // expand billboard
  out.uv = float2(in.uv.x, 1.0 - in.uv.y);
  
  out.color = uniforms.u_color;
  
  return out;
}
