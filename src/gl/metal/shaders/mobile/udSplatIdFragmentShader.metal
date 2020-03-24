#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_params
{
    float4 u_idOverride;
};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_params& u_params [[buffer(0)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
    main0_out out = {};
    float4 _42 = texture0.sample(sampler0, in.in_var_TEXCOORD0);
    float4 _46 = texture1.sample(sampler1, in.in_var_TEXCOORD0);
    float _51 = _42.w;
    float4 _59;
    if ((u_params.u_idOverride.w == 0.0) || (abs(u_params.u_idOverride.w - _51) <= 0.00150000001303851604461669921875))
    {
        _59 = float4(_51, 0.0, 0.0, 1.0);
    }
    else
    {
        _59 = float4(0.0);
    }
    out.out_var_SV_Target = _59;
    out.gl_FragDepth = _46.x;
    return out;
}

