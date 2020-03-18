#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_screenSize;
};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float2 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
    float4 in_var_COLOR0 [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _35 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float2 _43 = _35.xy + ((u_EveryObject.u_screenSize.xy * in.in_var_POSITION) * _35.w);
    out.gl_Position = float4(_43.x, _43.y, _35.z, _35.w);
    out.out_var_COLOR0 = in.in_var_COLOR0;
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    return out;
}

