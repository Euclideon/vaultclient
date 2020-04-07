#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4 u_colour;
    float4 u_thickness;
    float4x4 u_worldViewProjectionMatrix;
};

constant float2 _33 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 in_var_POSITION [[attribute(0)]];
    float4 in_var_COLOR0 [[attribute(1)]];
    float4 in_var_COLOR1 [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _47 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_POSITION.xyz, 1.0);
    float _48 = _47.w;
    float4 _50 = _47 / float4(_48);
    float2 _53 = (_50.xy * float2(0.5)) + float2(0.5);
    float4 _58 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_COLOR0.xyz, 1.0);
    float2 _64 = ((_58.xy / float2(_58.w)) * float2(0.5)) + float2(0.5);
    float4 _70 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_COLOR1.xyz, 1.0);
    float2 _76 = ((_70.xy / float2(_70.w)) * float2(0.5)) + float2(0.5);
    float2 _80 = _53;
    _80.x = _53.x * u_EveryObject.u_thickness.y;
    float4 _83 = float4(_76.x, _76.y, _70.z, _70.w);
    _83.x = _76.x * u_EveryObject.u_thickness.y;
    float4 _86 = float4(_64.x, _64.y, _58.z, _58.w);
    _86.x = _64.x * u_EveryObject.u_thickness.y;
    float2 _89 = normalize(_80 - _86.xy);
    float2 _92 = normalize(_83.xy - _80);
    float2 _94 = normalize(_89 + _92);
    float2 _108 = float2(-_94.y, _94.x) * ((u_EveryObject.u_thickness.x * 0.5) * (1.0 + pow(1.5 - (dot(_89, _92) * 0.5), 2.0)));
    float2 _111 = _108;
    _111.x = _108.x / u_EveryObject.u_thickness.y;
    float2 _119 = _33;
    _119.x = 1.0 + _48;
    out.gl_Position = _50 + float4(_111 * in.in_var_POSITION.w, 0.0, 0.0);
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = _119;
    return out;
}

