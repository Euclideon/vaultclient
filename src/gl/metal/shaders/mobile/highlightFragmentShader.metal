#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct HFSInput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float2 uv2;
  float2 uv3;
  float2 uv4;
};

struct HVSUniforms
{
  float4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
  float4 u_color;
};

fragment float4
main0(HFSInput in [[stage_in]], constant HVSUniforms& uniforms [[buffer(1)]], texture2d<float, access::sample> HFSimg [[texture(0)]], sampler HFSSampler [[sampler(0)]])
{
  float4 middle = HFSimg.sample(HFSSampler, in.uv0);
  float result = middle.w;
  
  // 'outside' the geometry, just use the blurred 'distance'
  if (middle.x == 0.0)
      return float4(uniforms.u_color.xyz, result * uniforms.u_stepSizeThickness.z * uniforms.u_color.a);
  
  result = 1.0 - result;
  
  // look for an edge, setting to full color if found
  float softenEdge = 0.15 * uniforms.u_color.a;
  result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv1).x - middle.x, -0.00001);
  result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv2).x - middle.x, -0.00001);
  result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv3).x - middle.x, -0.00001);
  result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv4).x - middle.x, -0.00001);
  
  result = max(uniforms.u_stepSizeThickness.w, result) * uniforms.u_color.w; // overlay color
  return float4(uniforms.u_color.xyz, result);
}
