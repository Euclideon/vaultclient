#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct PNUVSInput
{
  float3 pos [[attribute(0)]];
  float3 normal [[attribute(1)]];
  float2 uv [[attribute(2)]];
};

struct PNUVSOutput
{
  float4 pos [[position]];
  float2 uv;
  float3 normal;
  float4 color;
};

struct PNUVSUniforms1
{
  float4x4 u_worldViewProjectionMatrix;
  float4x4 u_worldMatrix;
  float4 u_color;
};

vertex PNUVSOutput
main0(PNUVSInput in [[stage_in]], constant PNUVSUniforms1& PNUVS1 [[buffer(1)]])
{
  PNUVSOutput out;

  // making the assumption that the model matrix won't contain non-uniform scale
  float3 worldNormal = normalize((PNUVS1.u_worldMatrix * float4(in.normal, 0.0)).xyz);

  out.pos = PNUVS1.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
  out.uv = in.uv;
  out.normal = worldNormal;
  out.color = PNUVS1.u_color;
  
  return out;
}
