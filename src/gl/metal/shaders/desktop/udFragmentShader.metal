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

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
    main0_out out = {};
    out.out_var_SV_Target = float4(texture0.sample(sampler0, in.in_var_TEXCOORD0).xyz, 1.0);
    out.gl_FragDepth = texture1.sample(sampler1, in.in_var_TEXCOORD0).x;
    return out;
}

