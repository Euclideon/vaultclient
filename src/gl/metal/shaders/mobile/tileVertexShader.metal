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

constant float2 _48 = {};

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

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]], texture2d<float> demTexture [[texture(0)]], sampler demSampler [[sampler(0)]])
{
    main0_out out = {};
    float2 _53 = in.in_var_POSITION.xy * 2.0;
    float _54 = _53.x;
    float _55 = floor(_54);
    float _56 = _53.y;
    float _57 = floor(_56);
    float _59 = fast::min(2.0, _55 + 1.0);
    float _62 = _54 - _55;
    float _64 = _57 * 3.0;
    int _66 = int(_64 + _55);
    float _73 = fast::min(2.0, _57 + 1.0) * 3.0;
    int _75 = int(_73 + _55);
    float4 _84 = u_EveryObject.u_eyePositions[_66] + ((u_EveryObject.u_eyePositions[int(_64 + _59)] - u_EveryObject.u_eyePositions[_66]) * _62);
    float4 _100 = demTexture.sample(demSampler, (u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy)), level(0.0));
    float4 _123 = u_EveryObject.u_projection * ((_84 + (((u_EveryObject.u_eyePositions[_75] + ((u_EveryObject.u_eyePositions[int(_73 + _59)] - u_EveryObject.u_eyePositions[_75]) * _62)) - _84) * (_56 - _57))) + ((u_EveryObject.u_view * float4(u_EveryObject.u_tileNormal.xyz * (((_100.x * 255.0) + (_100.y * 65280.0)) - 32768.0), 1.0)) - (u_EveryObject.u_view * float4(0.0, 0.0, 0.0, 1.0))));
    float2 _134 = _48;
    _134.x = 1.0 + _123.w;
    out.gl_Position = _123;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = _134;
    return out;
}

