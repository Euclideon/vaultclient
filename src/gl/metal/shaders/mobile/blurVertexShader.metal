#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4 u_stepSize;
};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float2 out_var_TEXCOORD1 [[user(locn1)]];
    float2 out_var_TEXCOORD2 [[user(locn2)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrame& u_EveryFrame [[buffer(0)]])
{
    main0_out out = {};
    float2 _36 = u_EveryFrame.u_stepSize.xy * 1.41999995708465576171875;
    out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0 - _36;
    out.out_var_TEXCOORD1 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD2 = in.in_var_TEXCOORD0 + _36;
    return out;
}

