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
    float4 out_var_SV_Target1 [[color(1)]];
};

struct main0_in
{
    float4 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_params& u_params [[buffer(1)]], texture2d<float> sceneDepthTexture [[texture(0)]], texture2d<float> shadowMapAtlasTexture [[texture(1)]], sampler sceneDepthSampler [[sampler(0)]], sampler shadowMapAtlasSampler [[sampler(1)]])
{
    main0_out out = {};
    float4 _62 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1);
    float _68 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float _74 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float4 _92 = u_params.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / _68) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _62.x * _74) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0);
    float4 _101 = float4((_92 / float4(_92.w)).xyz, 1.0);
    float4 _102 = u_params.u_shadowMapVP[0] * _101;
    float4 _105 = u_params.u_shadowMapVP[1] * _101;
    float4 _108 = u_params.u_shadowMapVP[2] * _101;
    float _110 = _102.w;
    float3 _114 = ((_102.xyz / float3(_110)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _116 = _105.w;
    float3 _120 = ((_105.xyz / float3(_116)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _122 = _108.w;
    float3 _126 = ((_108.xyz / float3(_122)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _127 = _114.x;
    float _131 = _114.y;
    float _136 = _114.z;
    float _142 = _120.x;
    float _146 = _120.y;
    float _151 = _120.z;
    float _157 = _126.x;
    float _161 = _126.y;
    float _166 = _126.z;
    float4 _188 = mix(mix(mix(float4(0.0), float4(_127 * 0.3333333432674407958984375, 1.0 - _131, _136, _110), float4(float((((((_127 >= 0.0) && (_127 <= 1.0)) && (_131 >= 0.0)) && (_131 <= 1.0)) && (_136 >= 0.0)) && (_136 <= 1.0)))), float4(0.3333333432674407958984375 + (_142 * 0.3333333432674407958984375), 1.0 - _146, _151, _116), float4(float((((((_142 >= 0.0) && (_142 <= 1.0)) && (_146 >= 0.0)) && (_146 <= 1.0)) && (_151 >= 0.0)) && (_151 <= 1.0)))), float4(0.666666686534881591796875 + (_157 * 0.3333333432674407958984375), 1.0 - _161, _166, _122), float4(float((((((_157 >= 0.0) && (_157 <= 1.0)) && (_161 >= 0.0)) && (_161 <= 1.0)) && (_166 >= 0.0)) && (_166 <= 1.0))));
    float4 _226;
    if ((length(_188.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_188.z * _68))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _226 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, float4(fast::clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _188.w) * (1.0 / _74)) - shadowMapAtlasTexture.sample(shadowMapAtlasSampler, _188.xy).x), 0.0, 1.0)));
    }
    else
    {
        _226 = float4(0.0);
    }
    out.out_var_SV_Target0 = float4(_226.xyz * _226.w, 1.0);
    out.out_var_SV_Target1 = float4(0.0);
    return out;
}

