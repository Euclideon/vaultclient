#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct WVSInput
{
  float2 pos [[attribute(0)]];
};

struct WVSOutput
{
  float4 pos [[position]];
  float2 uv0;
  float2 uv1;
  float4 fragEyePos;
  float3 color;
};

struct WVSUniforms1
{
  float4 u_time;
};

struct WVSUniforms2
{
  float4 u_colorAndSize;
  float4x4 u_worldViewMatrix;
  float4x4 u_worldViewProjectionMatrix;
};

vertex WVSOutput
main0(WVSInput in [[stage_in]], constant WVSUniforms1& uWVS1 [[buffer(1)]], constant WVSUniforms2& uWVS2 [[buffer(3)]])
{
  WVSOutput out;

  float uvScaleBodySize = uWVS2.u_colorAndSize.w; // packed here

  // scale the uvs with time
  float uvOffset = uWVS1.u_time.x * 0.0625;
  out.uv0 = uvScaleBodySize * in.pos.xy * float2(0.25, 0.25) - float2(uvOffset);
  out.uv1 = uvScaleBodySize * in.pos.yx * float2(0.50, 0.50) - float2(uvOffset, uvOffset * 0.75);

  out.fragEyePos = uWVS2.u_worldViewMatrix * float4(in.pos, 0.0, 1.0);
  out.color = uWVS2.u_colorAndSize.xyz;
  out.pos = uWVS2.u_worldViewProjectionMatrix * float4(in.pos, 0.0, 1.0);

  return out;
}
