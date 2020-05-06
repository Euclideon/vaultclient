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

constant float2 _44 = {};

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
    float2 _60 = _44;
    _60.y = in.in_var_COLOR0.w;
    float2 _92;
    float3 _93;
    if (((u_EveryFrame.u_options >> 2) & 1) == 0)
    {
        float2 _72 = _60;
        _72.x = (in.in_var_TEXCOORD0.y * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _92 = _72;
        _93 = float3(u_EveryFrame.u_worldUp.x, u_EveryFrame.u_worldUp.y, u_EveryFrame.u_worldUp.z);
    }
    else
    {
        float2 _87 = _60;
        _87.x = (in.in_var_TEXCOORD0.x * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _92 = _87;
        _93 = float3(in.in_var_COLOR0.xyz);
    }
    int _94 = u_EveryFrame.u_options & 15;
    float3 _120;
    if (_94 == 0)
    {
        _120 = in.in_var_POSITION + ((_93 * u_EveryFrame.u_width) * in.in_var_COLOR0.w);
    }
    else
    {
        float3 _119;
        if (_94 == 1)
        {
            _119 = in.in_var_POSITION - ((_93 * u_EveryFrame.u_width) * in.in_var_COLOR0.w);
        }
        else
        {
            _119 = in.in_var_POSITION + ((_93 * u_EveryFrame.u_width) * (in.in_var_COLOR0.w - 0.5));
        }
        _120 = _119;
    }
    float4 _127 = u_EveryObject.u_worldViewProjectionMatrix * float4(_120, 1.0);
    float2 _130 = _44;
    _130.x = 1.0 + _127.w;
    out.gl_Position = _127;
    out.out_var_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, float4(in.in_var_COLOR0.w));
    out.out_var_TEXCOORD0 = _92;
    out.out_var_TEXCOORD1 = _130;
    return out;
}

