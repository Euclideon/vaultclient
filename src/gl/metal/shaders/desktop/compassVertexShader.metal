#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    packed_float3 u_sunDirection;
    float _padding;
};

constant float2 _34 = {};

struct main0_out
{
    float3 out_var_COLOR0 [[user(locn0)]];
    float4 out_var_COLOR1 [[user(locn1)]];
    float3 out_var_COLOR2 [[user(locn2)]];
    float4 out_var_COLOR3 [[user(locn3)]];
    float2 out_var_TEXCOORD0 [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float3 in_var_NORMAL [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _44 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_POSITION, 1.0);
    float2 _53 = _34;
    _53.x = 1.0 + _44.w;
    out.gl_Position = _44;
    out.out_var_COLOR0 = (in.in_var_NORMAL * 0.5) + float3(0.5);
    out.out_var_COLOR1 = u_EveryObject.u_colour;
    out.out_var_COLOR2 = float3(u_EveryObject.u_sunDirection);
    out.out_var_COLOR3 = _44;
    out.out_var_TEXCOORD0 = _53;
    return out;
}

