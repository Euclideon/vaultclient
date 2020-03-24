#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

// This should match CPU struct size
#define VERTEX_COUNT 2

struct TVSInput
{
  float3 a_uv [[attribute(0)]];
};

struct PCU
{
  float4 v_position [[position]];
  float4 v_color;
  float2 v_uv;
};

struct TVSUniforms
{
  float4x4 u_projection;
  float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
  float4 u_color;
};

vertex PCU
main0(TVSInput in [[stage_in]], constant TVSUniforms& uTVS [[buffer(1)]])
{
  PCU out;
  
  // TODO: could have precision issues on some devices
  out.v_position = uTVS.u_projection * uTVS.u_eyePositions[int(in.a_uv.z)];
  
  out.v_uv = in.a_uv.xy;
  out.v_color = uTVS.u_color;
  return out;
}
