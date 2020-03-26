#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_vertParams
{
    float4x4 u_modelFromView;
    float4x4 u_viewFromClip;
};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float3 out_var_TEXCOORD1 [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_vertParams& u_vertParams [[buffer(0)]])
{
    main0_out out = {};
    float4 _34 = float4(in.in_var_POSITION, 1.0);
    out.gl_Position = _34;
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD1 = (u_vertParams.u_modelFromView * float4((u_vertParams.u_viewFromClip * _34).xyz, 0.0)).xyz;
    return out;
}

