#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrame
{
    float4x4 u_inverseViewProjection;
};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
};

struct main0_in
{
    float4 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrame& u_EveryFrame [[buffer(0)]], texture2d<float> albedoTexture [[texture(0)]], sampler albedoSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _43 = u_EveryFrame.u_inverseViewProjection * float4(in.in_var_TEXCOORD0.xy, 1.0, 1.0);
    float3 _48 = normalize(_43.xyz / float3(_43.w));
    out.out_var_SV_Target0 = albedoTexture.sample(albedoSampler, (float2(atan2(_48.x, _48.y) + 3.1415927410125732421875, acos(_48.z)) * float2(0.15915493667125701904296875, 0.3183098733425140380859375)));
    out.out_var_SV_Target1 = float4(0.0);
    return out;
}

