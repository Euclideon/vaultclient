#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrameVert
{
    float4 u_time;
};

struct type_u_EveryObject
{
    float4 u_colourAndSize;
    float4x4 u_worldViewMatrix;
    float4x4 u_worldViewProjectionMatrix;
};

constant float2 _42 = {};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float2 out_var_TEXCOORD1 [[user(locn1)]];
    float4 out_var_COLOR0 [[user(locn2)]];
    float3 out_var_COLOR1 [[user(locn3)]];
    float2 out_var_TEXCOORD2 [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrameVert& u_EveryFrameVert [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]])
{
    main0_out out = {};
    float _52 = u_EveryFrameVert.u_time.x * 0.0625;
    float4 _68 = float4(in.in_var_POSITION, 1.0);
    float4 _74 = u_EveryObject.u_worldViewProjectionMatrix * _68;
    float2 _77 = _42;
    _77.x = 1.0 + _74.w;
    out.gl_Position = _74;
    out.out_var_TEXCOORD0 = ((in.in_var_TEXCOORD0 * u_EveryObject.u_colourAndSize.w) * float2(0.25)) - float2(_52);
    out.out_var_TEXCOORD1 = ((in.in_var_TEXCOORD0.yx * u_EveryObject.u_colourAndSize.w) * float2(0.5)) - float2(_52, u_EveryFrameVert.u_time.x * 0.046875);
    out.out_var_COLOR0 = u_EveryObject.u_worldViewMatrix * _68;
    out.out_var_COLOR1 = u_EveryObject.u_colourAndSize.xyz;
    out.out_var_TEXCOORD2 = _77;
    return out;
}

