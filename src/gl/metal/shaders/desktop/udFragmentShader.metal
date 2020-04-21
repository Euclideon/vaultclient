#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> sceneColourTexture [[texture(0)]], texture2d<float> sceneDepthTexture [[texture(1)]], sampler sceneColourSampler [[sampler(0)]], sampler sceneDepthSampler [[sampler(1)]])
{
    main0_out out = {};
    float4 _34 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _35 = _34.x;
    float4 _40 = float4(sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0).zyx, 1.0);
    _40.w = _35;
    out.out_var_SV_Target = _40;
    out.gl_FragDepth = _35;
    return out;
}

