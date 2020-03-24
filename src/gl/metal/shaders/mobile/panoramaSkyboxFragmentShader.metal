#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4x4 u_inverseViewProjection;
};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float4 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrame& u_EveryFrame [[buffer(0)]], texture2d<float> u_texture [[texture(0)]], sampler sampler0 [[sampler(0)]])
{
    main0_out out = {};
    float4 _40 = u_EveryFrame.u_inverseViewProjection * float4(in.in_var_TEXCOORD0.xy, 1.0, 1.0);
    float3 _45 = normalize(_40.xyz / float3(_40.w));
    out.out_var_SV_Target = u_texture.sample(sampler0, (float2(atan2(_45.x, _45.y) + 3.1415927410125732421875, acos(_45.z)) * float2(0.15915493667125701904296875, 0.3183098733425140380859375)));
    return out;
}

