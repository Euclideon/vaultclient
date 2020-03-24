#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_projection;
    float4 u_eyePositions[9];
    float4 u_colour;
    float4 u_uvOffsetScale;
};

constant float2 _31 = {};

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
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _40 = u_EveryObject.u_projection * u_EveryObject.u_eyePositions[int(in.in_var_POSITION.z)];
    float2 _52 = _31;
    _52.x = 1.0 + _40.w;
    out.gl_Position = _40;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = _52;
    return out;
}

