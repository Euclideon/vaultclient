#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct PEVParams
{
  float4 u_screenParams;
  float4 u_saturation;
};

struct PEVOutput
{
  float2 out_var_TEXCOORD0 [[user(locn0)]];
  float2 out_var_TEXCOORD1 [[user(locn1)]];
  float2 out_var_TEXCOORD2 [[user(locn2)]];
  float2 out_var_TEXCOORD3 [[user(locn3)]];
  float2 out_var_TEXCOORD4 [[user(locn4)]];
  float2 out_var_TEXCOORD5 [[user(locn5)]];
  float out_var_TEXCOORD6 [[user(locn6)]];
  float4 gl_Position [[position]];
};

struct PEVInput
{
  float3 in_var_POSITION [[attribute(0)]];
  float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex PEVOutput main0(PEVInput in [[stage_in]], constant PEVParams& u_params [[buffer(0)]])
{
  PEVOutput out = {};
  out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
  out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
  out.out_var_TEXCOORD1 = in.in_var_TEXCOORD0 + u_params.u_screenParams.xy;
  out.out_var_TEXCOORD2 = in.in_var_TEXCOORD0 - u_params.u_screenParams.xy;
  out.out_var_TEXCOORD3 = in.in_var_TEXCOORD0 + float2(u_params.u_screenParams.x, -u_params.u_screenParams.y);
  out.out_var_TEXCOORD4 = in.in_var_TEXCOORD0 + float2(-u_params.u_screenParams.x, u_params.u_screenParams.y);
  out.out_var_TEXCOORD5 = u_params.u_screenParams.xy;
  out.out_var_TEXCOORD6 = u_params.u_saturation.x;
  return out;
}
