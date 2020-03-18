#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_vertParams
{
    float4 u_outlineStepSize;
};

struct main0_out
{
    float4 out_var_TEXCOORD0 [[user(locn0)]];
    float2 out_var_TEXCOORD1 [[user(locn1)]];
    float2 out_var_TEXCOORD2 [[user(locn2)]];
    float2 out_var_TEXCOORD3 [[user(locn3)]];
    float2 out_var_TEXCOORD4 [[user(locn4)]];
    float2 out_var_TEXCOORD5 [[user(locn5)]];
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
    float4 _34 = float4(in.in_var_POSITION.xy, 0.0, 1.0);
    float3 _39 = float3(u_vertParams.u_outlineStepSize.xy, 0.0);
    float2 _40 = _39.xz;
    float2 _43 = _39.zy;
    out.gl_Position = _34;
    out.out_var_TEXCOORD0 = _34;
    out.out_var_TEXCOORD1 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD2 = in.in_var_TEXCOORD0 + _40;
    out.out_var_TEXCOORD3 = in.in_var_TEXCOORD0 - _40;
    out.out_var_TEXCOORD4 = in.in_var_TEXCOORD0 + _43;
    out.out_var_TEXCOORD5 = in.in_var_TEXCOORD0 - _43;
    return out;
}

