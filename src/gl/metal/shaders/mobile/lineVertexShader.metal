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
    float4 _46 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_POSITION.xyz, 1.0);
    float _47 = _46.w;
    float4 _49 = _46 / float4(_47);
    float2 _52 = (_49.xy * float2(0.5)) + float2(0.5);
    float4 _57 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_COLOR0.xyz, 1.0);
    float2 _63 = ((_57.xy / float2(_57.w)) * float2(0.5)) + float2(0.5);
    float4 _69 = u_EveryObject.u_worldViewProjectionMatrix * float4(in.in_var_COLOR1.xyz, 1.0);
    float2 _75 = ((_69.xy / float2(_69.w)) * float2(0.5)) + float2(0.5);
    float2 _79 = _52;
    _79.x = _52.x * u_EveryObject.u_thickness.y;
    float4 _82 = float4(_75.x, _75.y, _69.z, _69.w);
    _82.x = _75.x * u_EveryObject.u_thickness.y;
    float4 _85 = float4(_63.x, _63.y, _57.z, _57.w);
    _85.x = _63.x * u_EveryObject.u_thickness.y;
    float2 _88 = normalize(_79 - _85.xy);
    float2 _91 = normalize(_82.xy - _79);
    float2 _93 = normalize(_88 + _91);
    float2 _108 = float2(-_93.y, _93.x) * ((u_EveryObject.u_thickness.x * 0.5) * (1.0 + (pow(0.5 - (dot(_88, _91) * 0.5), 7.0) * 7.0)));
    float2 _111 = _108;
    _111.x = _108.x / u_EveryObject.u_thickness.y;
    float2 _119 = _33;
    _119.x = 1.0 + _47;
    out.gl_Position = _49 + float4(_111 * in.in_var_POSITION.w, 0.0, 0.0);
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = _119;
    return out;
}

