#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4x4 u_projection;
    float4x4 u_view;
    float4 u_eyePositions[9];
    float4 u_colour;
    float4 u_uvOffsetScale;
    float4 u_demUVOffsetScale;
    float4 u_tileNormal;
};

constant float2 _47 = {};

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

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]], texture2d<float> texture1 [[texture(0)]], sampler sampler1 [[sampler(0)]])
{
    main0_out out = {};
    float2 _51 = in.in_var_POSITION.xy * 2.0;
    float _52 = _51.x;
    float _53 = floor(_52);
    float _54 = _51.y;
    float _55 = floor(_54);
    float _57 = fast::min(2.0, _53 + 1.0);
    float _60 = _52 - _53;
    float _62 = _55 * 3.0;
    int _64 = int(_62 + _53);
    float _71 = fast::min(2.0, _55 + 1.0) * 3.0;
    int _73 = int(_71 + _53);
    float4 _82 = u_EveryObject.u_eyePositions[_64] + ((u_EveryObject.u_eyePositions[int(_62 + _57)] - u_EveryObject.u_eyePositions[_64]) * _60);
    float4 _117 = u_EveryObject.u_projection * ((_82 + (((u_EveryObject.u_eyePositions[_73] + ((u_EveryObject.u_eyePositions[int(_71 + _57)] - u_EveryObject.u_eyePositions[_73]) * _60)) - _82) * (_54 - _55))) + ((u_EveryObject.u_view * float4(u_EveryObject.u_tileNormal.xyz * (texture1.sample(sampler1, (u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy)), level(0.0)).x * 32768.0), 1.0)) - (u_EveryObject.u_view * float4(0.0, 0.0, 0.0, 1.0))));
    float2 _128 = _47;
    _128.x = 1.0 + _117.w;
    out.gl_Position = _117;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = _128;
    return out;
}

