#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4 u_bottomColour;
    float4 u_topColour;
    float u_orientation;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
    float _padding1;
    float2 _padding2;
};

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
};

constant float2 _41 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
    float4 in_var_COLOR0 [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrame& u_EveryFrame [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]])
{
    main0_out out = {};
    float4 _82 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_POSITION + mix(float3(0.0, 0.0, in.in_var_COLOR0.w) * u_EveryFrame.u_width, in.in_var_COLOR0.xyz, float3(u_EveryFrame.u_orientation)), 1.0);
    float2 _85 = _41;
    _85.x = 1.0 + _82.w;
    out.gl_Position = _82;
    out.out_var_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, float4(in.in_var_COLOR0.w));
    out.out_var_TEXCOORD0 = float2((mix(in.in_var_TEXCOORD0.y, in.in_var_TEXCOORD0.x, u_EveryFrame.u_orientation) * u_EveryFrame.u_textureRepeatScale) - (u_EveryFrame.u_time * u_EveryFrame.u_textureScrollSpeed), in.in_var_COLOR0.w);
    out.out_var_TEXCOORD1 = _85;
    return out;
}

