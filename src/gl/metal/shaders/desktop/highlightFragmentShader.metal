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
    float4 _103;
    switch (0u)
    {
        default:
        {
            float4 _49 = colourTexture.sample(colourSampler, in.in_var_TEXCOORD0);
            float _50 = _49.x;
            if (_50 == 0.0)
            {
                _103 = float4(in.in_var_COLOR0.xyz, fast::min(1.0, (_49.y * in.in_var_COLOR1.z) * in.in_var_COLOR0.w));
                break;
            }
            float _67 = 0.1500000059604644775390625 * in.in_var_COLOR0.w;
            _103 = float4(in.in_var_COLOR0.xyz, fast::max(in.in_var_COLOR1.w, ((((1.0 - _49.y) + (_67 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD1).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD2).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD3).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(colourTexture.sample(colourSampler, in.in_var_TEXCOORD4).x - _50, -9.9999997473787516355514526367188e-06))) * in.in_var_COLOR0.w);
            break;
        }
    }
    out.out_var_SV_Target0 = _103;
    out.out_var_SV_Target1 = float4(0.0);
    return out;
}

