#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
};

struct main0_in
{
    float4 in_var_COLOR0 [[user(locn0)]];
    float2 in_var_TEXCOORD0 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> colourTexture [[texture(0)]], sampler colourSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _31 = colourTexture.sample(colourSampler, in.in_var_TEXCOORD0);
    out.out_var_SV_Target0 = float4(_31.xyz * in.in_var_COLOR0.xyz, _31.w * in.in_var_COLOR0.w);
    out.out_var_SV_Target1 = float4(0.0);
    return out;
}

