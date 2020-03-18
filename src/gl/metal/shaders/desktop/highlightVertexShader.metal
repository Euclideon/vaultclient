#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4 u_stepSizeThickness;
    float4 u_colour;
};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float2 out_var_TEXCOORD1 [[user(locn1)]];
    float2 out_var_TEXCOORD2 [[user(locn2)]];
    float2 out_var_TEXCOORD3 [[user(locn3)]];
    float2 out_var_TEXCOORD4 [[user(locn4)]];
    float4 out_var_COLOR0 [[user(locn5)]];
    float4 out_var_COLOR1 [[user(locn6)]];
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
    out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD1 = in.in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * float2(-1.0));
    out.out_var_TEXCOORD2 = in.in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * float2(1.0, -1.0));
    out.out_var_TEXCOORD3 = in.in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * float2(-1.0, 1.0));
    out.out_var_TEXCOORD4 = in.in_var_TEXCOORD0 + u_EveryFrame.u_stepSizeThickness.xy;
    out.out_var_COLOR0 = u_EveryFrame.u_colour;
    out.out_var_COLOR1 = u_EveryFrame.u_stepSizeThickness;
    return out;
}

