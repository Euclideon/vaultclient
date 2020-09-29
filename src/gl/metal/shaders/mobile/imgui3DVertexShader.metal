#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_screenSize;
};

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
    float4 _44 = u_EveryObject.u_worldViewProjectionMatrix * float4(0.0, 0.0, 0.0, 1.0);
    float _49 = _44.w;
    float2 _52 = _44.xy + ((u_EveryObject.u_screenSize.xy * in.in_var_POSITION) * _49);
    float2 _65;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[1u][3u] == 0.0)
            {
                _65 = float2(_44.z, _49);
                break;
            }
            _65 = float2(1.0 + _49, 0.0);
            break;
        }
    }
    out.gl_Position = float4(_52.x, _52.y, _44.z, _44.w);
    out.out_var_COLOR0 = in.in_var_COLOR0;
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    out.out_var_TEXCOORD1 = _65;
    out.out_var_TEXCOORD2 = float2(u_EveryObject.u_screenSize.w);
    return out;
}

