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

struct type_u_params
{
    float4x4 u_shadowMapVP[3];
    float4x4 u_inverseProjection;
    float4 u_visibleColour;
    float4 u_notVisibleColour;
    float4 u_viewDistance;
};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
};

struct main0_in
{
    float4 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_params& u_params [[buffer(1)]], texture2d<float> sceneDepthTexture [[texture(0)]], texture2d<float> shadowMapAtlasTexture [[texture(1)]], sampler sceneDepthSampler [[sampler(0)]], sampler shadowMapAtlasSampler [[sampler(1)]])
{
    main0_out out = {};
    float4 _61 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1);
    float _67 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float _73 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float4 _91 = u_params.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / _67) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _61.x * _73) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0);
    float4 _100 = float4((_91 / float4(_91.w)).xyz, 1.0);
    float4 _101 = u_params.u_shadowMapVP[0] * _100;
    float4 _104 = u_params.u_shadowMapVP[1] * _100;
    float4 _107 = u_params.u_shadowMapVP[2] * _100;
    float _109 = _101.w;
    float3 _113 = ((_101.xyz / float3(_109)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _115 = _104.w;
    float3 _119 = ((_104.xyz / float3(_115)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _121 = _107.w;
    float3 _125 = ((_107.xyz / float3(_121)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _126 = _113.x;
    float _130 = _113.y;
    float _135 = _113.z;
    float _141 = _119.x;
    float _145 = _119.y;
    float _150 = _119.z;
    float _156 = _125.x;
    float _160 = _125.y;
    float _165 = _125.z;
    float4 _187 = mix(mix(mix(float4(0.0), float4(_126 * 0.3333333432674407958984375, 1.0 - _130, _135, _109), float4(float((((((_126 >= 0.0) && (_126 <= 1.0)) && (_130 >= 0.0)) && (_130 <= 1.0)) && (_135 >= 0.0)) && (_135 <= 1.0)))), float4(0.3333333432674407958984375 + (_141 * 0.3333333432674407958984375), 1.0 - _145, _150, _115), float4(float((((((_141 >= 0.0) && (_141 <= 1.0)) && (_145 >= 0.0)) && (_145 <= 1.0)) && (_150 >= 0.0)) && (_150 <= 1.0)))), float4(0.666666686534881591796875 + (_156 * 0.3333333432674407958984375), 1.0 - _160, _165, _121), float4(float((((((_156 >= 0.0) && (_156 <= 1.0)) && (_160 >= 0.0)) && (_160 <= 1.0)) && (_165 >= 0.0)) && (_165 <= 1.0))));
    float4 _225;
    if ((length(_187.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_187.z * _67))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _225 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, float4(fast::clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _187.w) * (1.0 / _73)) - shadowMapAtlasTexture.sample(shadowMapAtlasSampler, _187.xy).x), 0.0, 1.0)));
    }
    else
    {
        _225 = float4(0.0);
    }
    out.out_var_SV_Target0 = float4(_225.xyz * _225.w, 1.0);
    return out;
}

