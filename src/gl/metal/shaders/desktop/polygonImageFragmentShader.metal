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
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float2 in_var_TEXCOORD1 [[user(locn3)]];
    float2 in_var_TEXCOORD2 [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], texture2d<float> albedoTexture [[texture(0)]], sampler albedoSampler [[sampler(0)]])
{
    main0_out out = {};
    float _56 = log2(in.in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    float4 _64 = float4(in.in_var_TEXCOORD2.x, ((step(0.0, 0.0) * 2.0) - 1.0) * _56, 0.0, 0.0);
    _64.w = in.in_var_TEXCOORD2.y;
    out.out_var_SV_Target0 = albedoTexture.sample(albedoSampler, in.in_var_TEXCOORD0) * in.in_var_COLOR0;
    out.out_var_SV_Target1 = _64;
    out.gl_FragDepth = _56;
    return out;
}

