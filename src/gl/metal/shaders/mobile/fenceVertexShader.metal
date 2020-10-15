#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4 u_bottomColour;
    float4 u_topColour;
    float4 u_worldUp;
    int u_options;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
    float _padding1;
    float _padding2;
    float _padding3;
};

struct type_u_EveryObject
{
    float4x4 u_worldViewProjectionMatrix;
};

constant float2 _49 = {};

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
    float2 _65 = _49;
    _65.y = in.in_var_COLOR0.w;
    float2 _97;
    float3 _98;
    if (((u_EveryFrame.u_options >> 2) & 1) == 0)
    {
        float2 _77 = _65;
        _77.x = (in.in_var_TEXCOORD0.y * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _97 = _77;
        _98 = float3(u_EveryFrame.u_worldUp.x, u_EveryFrame.u_worldUp.y, u_EveryFrame.u_worldUp.z);
    }
    else
    {
        float2 _92 = _65;
        _92.x = (in.in_var_TEXCOORD0.x * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _97 = _92;
        _98 = float3(in.in_var_COLOR0.xyz);
    }
    int _99 = u_EveryFrame.u_options & 15;
    float3 _125;
    if (_99 == 0)
    {
        _125 = in.in_var_POSITION + ((_98 * u_EveryFrame.u_width) * in.in_var_COLOR0.w);
    }
    else
    {
        float3 _124;
        if (_99 == 1)
        {
            _124 = in.in_var_POSITION - ((_98 * u_EveryFrame.u_width) * in.in_var_COLOR0.w);
        }
        else
        {
            _124 = in.in_var_POSITION + ((_98 * u_EveryFrame.u_width) * (in.in_var_COLOR0.w - 0.5));
        }
        _125 = _124;
    }
    float4 _132 = u_EveryObject.u_worldViewProjectionMatrix * float4(_125, 1.0);
    float2 _146;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[1u][3u] == 0.0)
            {
                _146 = float2(_132.zw);
                break;
            }
            _146 = float2(1.0 + _132.w, 0.0);
            break;
        }
    }
    out.gl_Position = _132;
    out.out_var_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, float4(in.in_var_COLOR0.w));
    out.out_var_TEXCOORD0 = _97;
    out.out_var_TEXCOORD1 = _146;
    return out;
}

