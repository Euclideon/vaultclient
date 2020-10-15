#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize;
};

constant float2 _39 = {};

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
    float4 _45 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float _51 = _45.w;
    float2 _57 = _45.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _51)) * in.in_var_POSITION.xy);
    float2 _72;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[1u][3u] == 0.0)
            {
                _72 = float2(_45.z, _51);
                break;
            }
            _72 = float2(1.0 + _51, 0.0);
            break;
        }
    }
    float2 _75 = _39;
    _75.x = u_EveryObject.u_screenSize.w;
    out.gl_Position = float4(_57.x, _57.y, _45.z, _45.w);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD1 = _72;
    out.out_var_TEXCOORD2 = _75;
    return out;
}

