#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryFrameFrag
{
    float4 u_specularDir;
    float4x4 u_eyeNormalMatrix;
    float4x4 u_inverseViewMatrix;
};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float4 in_var_COLOR1 [[user(locn3)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_EveryFrameFrag& u_EveryFrameFrag [[buffer(0)]], texture2d<float> u_normalMap [[texture(0)]], texture2d<float> u_skybox [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
    main0_out out = {};
    float3 _77 = normalize(((u_normalMap.sample(sampler0, in.in_var_TEXCOORD0).xyz * float3(2.0)) - float3(1.0)) + ((u_normalMap.sample(sampler0, in.in_var_TEXCOORD1).xyz * float3(2.0)) - float3(1.0)));
    float3 _79 = normalize(in.in_var_COLOR0.xyz);
    float3 _95 = normalize((u_EveryFrameFrag.u_eyeNormalMatrix * float4(_77, 0.0)).xyz);
    float3 _97 = normalize(reflect(_79, _95));
    float3 _121 = normalize((u_EveryFrameFrag.u_inverseViewMatrix * float4(_97, 0.0)).xyz);
    out.out_var_SV_Target = float4(mix(u_skybox.sample(sampler1, (float2(atan2(_121.x, _121.y) + 3.1415927410125732421875, acos(_121.z)) * float2(0.15915493667125701904296875, 0.3183098733425140380859375))).xyz, in.in_var_COLOR1.xyz * mix(float3(1.0, 1.0, 0.60000002384185791015625), float3(0.3499999940395355224609375), float3(pow(fast::max(0.0, _77.z), 5.0))), float3(((dot(_95, _79) * (-0.5)) + 0.5) * 0.75)) + float3(pow(abs(dot(_97, normalize((u_EveryFrameFrag.u_eyeNormalMatrix * float4(normalize(u_EveryFrameFrag.u_specularDir.xyz), 0.0)).xyz))), 50.0) * 0.5), 1.0);
    return out;
}

