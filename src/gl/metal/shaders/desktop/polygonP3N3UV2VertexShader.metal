#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4x4 u_worldMatrix;
    float4 u_colour;
    float4 u_objectInfo;
};

constant float2 _36 = {};

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float3 out_var_NORMAL [[user(locn1)]];
    float4 out_var_COLOR0 [[user(locn2)]];
    float2 out_var_TEXCOORD1 [[user(locn3)]];
    float2 out_var_TEXCOORD2 [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float3 in_var_NORMAL [[attribute(1)]];
    float2 in_var_TEXCOORD0 [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _56 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_POSITION, 1.0);
    float2 _61 = _36;
    _61.x = 1.0 + _56.w;
    out.gl_Position = _56;
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_NORMAL = normalize((u_EveryObject.u_worldMatrix * float4(in.in_var_NORMAL, 0.0)).xyz);
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD1 = _61;
    out.out_var_TEXCOORD2 = u_EveryObject.u_objectInfo.xy;
    return out;
}

