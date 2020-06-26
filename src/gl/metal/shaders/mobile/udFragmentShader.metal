#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> sceneColourTexture [[texture(0)]], texture2d<float> sceneDepthTexture [[texture(1)]], sampler sceneColourSampler [[sampler(0)]], sampler sceneDepthSampler [[sampler(1)]])
{
    main0_out out = {};
    float4 _38 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _39 = _38.x;
    out.out_var_SV_Target0 = float4(sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0).zyx, 1.0);
    out.out_var_SV_Target1 = float4(0.0, ((step(0.0, 0.0) * 2.0) - 1.0) * _39, 0.0, 0.0);
    out.gl_FragDepth = _39;
    return out;
}

