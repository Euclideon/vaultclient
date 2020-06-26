#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
};

struct type_u_EveryFrameFrag
{
    float4 u_specularDir;
    float4x4 u_eyeNormalMatrix;
    float4x4 u_inverseViewMatrix;
};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float4 in_var_COLOR1 [[user(locn3)]];
    float2 in_var_TEXCOORD2 [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_EveryFrameFrag& u_EveryFrameFrag [[buffer(1)]], texture2d<float> normalMapTexture [[texture(0)]], texture2d<float> skyboxTexture [[texture(1)]], sampler normalMapSampler [[sampler(0)]], sampler skyboxSampler [[sampler(1)]])
{
    main0_out out = {};
    float3 _91 = normalize(((normalMapTexture.sample(normalMapSampler, in.in_var_TEXCOORD0).xyz * float3(2.0)) - float3(1.0)) + ((normalMapTexture.sample(normalMapSampler, in.in_var_TEXCOORD1).xyz * float3(2.0)) - float3(1.0)));
    float3 _93 = normalize(in.in_var_COLOR0.xyz);
    float3 _109 = normalize((u_EveryFrameFrag.u_eyeNormalMatrix * float4(_91, 0.0)).xyz);
    float3 _111 = normalize(reflect(_93, _109));
    float3 _135 = normalize((u_EveryFrameFrag.u_inverseViewMatrix * float4(_111, 0.0)).xyz);
    float _163 = log2(in.in_var_TEXCOORD2.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out.out_var_SV_Target0 = (float4(mix(skyboxTexture.sample(skyboxSampler, (float2(atan2(_135.x, _135.y) + 3.1415927410125732421875, acos(_135.z)) * float2(0.15915493667125701904296875, 0.3183098733425140380859375))).xyz, in.in_var_COLOR1.xyz * mix(float3(1.0, 1.0, 0.60000002384185791015625), float3(0.3499999940395355224609375), float3(pow(fast::max(0.0, _91.z), 5.0))), float3(((dot(_109, _93) * (-0.5)) + 0.5) * 0.75)) + float3(pow(abs(dot(_111, normalize((u_EveryFrameFrag.u_eyeNormalMatrix * float4(normalize(u_EveryFrameFrag.u_specularDir.xyz), 0.0)).xyz))), 50.0) * 0.5), 1.0) * 0.300000011920928955078125) + float4(0.20000000298023223876953125, 0.4000000059604644775390625, 0.699999988079071044921875, 1.0);
    out.out_var_SV_Target1 = float4(0.0, ((step(0.0, 0.0) * 2.0) - 1.0) * _163, 0.0, 0.0);
    out.gl_FragDepth = _163;
    return out;
}

