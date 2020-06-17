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

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
};

struct main0_in
{
    float4 in_var_COLOR0 [[user(locn0)]];
    float2 in_var_TEXCOORD0 [[user(locn1)]];
    float2 in_var_TEXCOORD1 [[user(locn2)]];
    float2 in_var_TEXCOORD2 [[user(locn3)]];
    float2 in_var_TEXCOORD3 [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], texture2d<float> colourTexture [[texture(0)]], texture2d<float> normalTexture [[texture(1)]], sampler colourSampler [[sampler(0)]], sampler normalSampler [[sampler(1)]])
{
    main0_out out = {};
    float3 _61 = (float3(normalTexture.sample(normalSampler, in.in_var_TEXCOORD3).xyz) * float3(2.0)) - float3(1.0);
    out.out_var_SV_Target0 = float4(colourTexture.sample(colourSampler, in.in_var_TEXCOORD0).xyz * in.in_var_COLOR0.xyz, in.in_var_COLOR0.w);
    out.out_var_SV_Target1 = float4(_61.xy, in.in_var_TEXCOORD2.x, ((step(0.0, _61.z) * 2.0) - 1.0) * (((in.in_var_TEXCOORD1.x / in.in_var_TEXCOORD1.y) * (1.0 / (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear))) + (u_cameraPlaneParams.u_clipZNear * (-0.5))));
    return out;
}

