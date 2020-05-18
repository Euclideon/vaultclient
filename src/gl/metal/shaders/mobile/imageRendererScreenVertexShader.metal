#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize;
};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float4 out_var_COLOR0 [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _36 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float2 _48 = _36.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _36.w)) * in.in_var_POSITION.xy);
    out.gl_Position = float4(_48.x, _48.y, _36.z, _36.w);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    return out;
}

