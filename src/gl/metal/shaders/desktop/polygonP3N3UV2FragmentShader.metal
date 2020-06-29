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

constant float _43 = {};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float3 in_var_NORMAL [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float2 in_var_TEXCOORD1 [[user(locn3)]];
    float2 in_var_TEXCOORD2 [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], texture2d<float> albedoTexture [[texture(0)]], sampler albedoSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _54 = albedoTexture.sample(albedoSampler, in.in_var_TEXCOORD0) * in.in_var_COLOR0;
    float _73 = log2(in.in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    float4 _83 = float4(in.in_var_TEXCOORD2.x, ((step(0.0, in.in_var_NORMAL.z) * 2.0) - 1.0) * _73, in.in_var_NORMAL.x, _43);
    _83.w = in.in_var_TEXCOORD2.y;
    out.out_var_SV_Target0 = float4(_54.xyz * ((dot(in.in_var_NORMAL, normalize(float3(0.85000002384185791015625, 0.1500000059604644775390625, 0.5))) * 0.5) + 0.5), _54.w);
    out.out_var_SV_Target1 = _83;
    out.gl_FragDepth = _73;
    return out;
}

