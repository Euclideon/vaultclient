#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct HVSInput
{
  float3 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
};

struct HFSInput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float2 uv2;
  float2 uv3;
  float2 uv4;
};

constant static float2 searchKernel[4] = {float2(-1, -1), float2(1, -1), float2(-1,  1), float2(1, 1)};

struct HVSUniforms
{
  float4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
  float4 u_color;
};

vertex HFSInput
main0(HVSInput in [[stage_in]], constant HVSUniforms& uniforms [[buffer(1)]])
{
  HFSInput out;
  
  out.pos = float4(in.pos.x, in.pos.y, 0.0, 1.0);
  
  out.uv0 = in.uv;
  out.uv1 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[0];
  out.uv2 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[1];
  out.uv3 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[2];
  out.uv4 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[3];
  
  return out;
}
