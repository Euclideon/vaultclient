#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

constant float _23 = {};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float4 in_var_COLOR0 [[user(locn0)]];
    float2 in_var_TEXCOORD0 [[user(locn1)]];
    float2 in_var_TEXCOORD1 [[user(locn2)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> colourTexture [[texture(0)]], sampler colourSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _42 = float4(colourTexture.sample(colourSampler, in.in_var_TEXCOORD0).xyz * in.in_var_COLOR0.xyz, _23);
    _42.w = in.in_var_TEXCOORD1.x / in.in_var_TEXCOORD1.y;
    out.out_var_SV_Target = _42;
    return out;
}

