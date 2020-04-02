#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float4 in_var_COLOR0 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> albedoTexture [[texture(0)]], sampler albedoSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _30 = albedoTexture.sample(albedoSampler, in.in_var_TEXCOORD0);
    float _33 = fast::min(_30.w, in.in_var_COLOR0.w);
    out.out_var_SV_Target = float4((_30.xyz * _33) + (in.in_var_COLOR0.xyz * (1.0 - _33)), 1.0);
    return out;
}

