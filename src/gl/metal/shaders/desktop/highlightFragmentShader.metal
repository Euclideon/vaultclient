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
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float2 in_var_TEXCOORD2 [[user(locn2)]];
    float2 in_var_TEXCOORD3 [[user(locn3)]];
    float2 in_var_TEXCOORD4 [[user(locn4)]];
    float4 in_var_COLOR0 [[user(locn5)]];
    float4 in_var_COLOR1 [[user(locn6)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> colourTexture [[texture(0)]], sampler colourSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _101;
    switch (0u)
    {
        default:
        {
            float4 _47 = colourTexture.sample(colourSampler, in.in_var_TEXCOORD0);
            float _48 = _47.x;
            if (_48 == 0.0)
            {
                _101 = float4(in.in_var_COLOR0.xyz, fast::min(1.0, (_47.y * in.in_var_COLOR1.z) * in.in_var_COLOR0.w));
                break;
            }
            float _65 = 0.1500000059604644775390625 * in.in_var_COLOR0.w;
            _101 = float4(in.in_var_COLOR0.xyz, fast::max(in.in_var_COLOR1.w, ((((1.0 - _47.y) + (_65 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD1).x - _48, -9.9999997473787516355514526367188e-06))) + (_65 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD2).x - _48, -9.9999997473787516355514526367188e-06))) + (_65 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD3).x - _48, -9.9999997473787516355514526367188e-06))) + (_65 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD4).x - _48, -9.9999997473787516355514526367188e-06))) * in.in_var_COLOR0.w);
            break;
        }
    }
    out.out_var_SV_Target = _101;
    return out;
}

