#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_screenSize;
};

constant float2 _33 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float2 out_var_TEXCOORD2 [[user(locn3)]];
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
    float4 _40 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float _45 = _40.w;
    float2 _48 = _40.xy + ((u_EveryObject.u_screenSize.xy * in.in_var_POSITION) * _45);
    float2 _51 = _33;
    _51.x = 1.0 + _45;
    out.gl_Position = float4(_48.x, _48.y, _40.z, _40.w);
    out.out_var_COLOR0 = in.in_var_COLOR0;
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD1 = _51;
    out.out_var_TEXCOORD2 = float2(u_EveryObject.u_screenSize.w);
    return out;
}

