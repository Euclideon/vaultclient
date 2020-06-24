#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize;
};

constant float2 _34 = {};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float4 out_var_COLOR0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float2 out_var_TEXCOORD2 [[user(locn3)]];
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
    float4 _40 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float _46 = _40.w;
    float2 _52 = _40.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _46)) * in.in_var_POSITION.xy);
    float2 _57 = _34;
    _57.x = 1.0 + _46;
    float2 _60 = _34;
    _60.x = u_EveryObject.u_screenSize.w;
    out.gl_Position = float4(_52.x, _52.y, _40.z, _40.w);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD1 = _57;
    out.out_var_TEXCOORD2 = _60;
    return out;
}

