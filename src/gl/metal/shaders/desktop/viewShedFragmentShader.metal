#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_unused1;
    float u_unused2;
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
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_params& u_params [[buffer(1)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
    main0_out out = {};
    float4 _59 = texture0.sample(sampler0, in.in_var_TEXCOORD0);
    float _65 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float _71 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float4 _87 = u_params.u_inverseProjection * float4((in.in_var_TEXCOORD0.x * 2.0) - 1.0, ((1.0 - in.in_var_TEXCOORD0.y) * 2.0) - 1.0, (u_cameraPlaneParams.s_CameraFarPlane / _65) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _59.x * _71) - 1.0)), 1.0);
    float4 _96 = float4((_87 / float4(_87.w)).xyz, 1.0);
    float4 _97 = u_params.u_shadowMapVP[0] * _96;
    float4 _100 = u_params.u_shadowMapVP[1] * _96;
    float4 _103 = u_params.u_shadowMapVP[2] * _96;
    float _105 = _97.w;
    float3 _109 = ((_97.xyz / float3(_105)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _111 = _100.w;
    float3 _115 = ((_100.xyz / float3(_111)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _117 = _103.w;
    float3 _121 = ((_103.xyz / float3(_117)) * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
    float _122 = _109.x;
    float _126 = _109.y;
    float _131 = _109.z;
    float _137 = _115.x;
    float _141 = _115.y;
    float _146 = _115.z;
    float _152 = _121.x;
    float _156 = _121.y;
    float _161 = _121.z;
    float4 _183 = mix(mix(mix(float4(0.0), float4(_122 * 0.3333333432674407958984375, 1.0 - _126, _131, _105), float4(float((((((_122 >= 0.0) && (_122 <= 1.0)) && (_126 >= 0.0)) && (_126 <= 1.0)) && (_131 >= 0.0)) && (_131 <= 1.0)))), float4(0.3333333432674407958984375 + (_137 * 0.3333333432674407958984375), 1.0 - _141, _146, _111), float4(float((((((_137 >= 0.0) && (_137 <= 1.0)) && (_141 >= 0.0)) && (_141 <= 1.0)) && (_146 >= 0.0)) && (_146 <= 1.0)))), float4(0.666666686534881591796875 + (_152 * 0.3333333432674407958984375), 1.0 - _156, _161, _117), float4(float((((((_152 >= 0.0) && (_152 <= 1.0)) && (_156 >= 0.0)) && (_156 <= 1.0)) && (_161 >= 0.0)) && (_161 <= 1.0))));
    float4 _221;
    if ((length(_183.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_183.z * _65))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _221 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, float4(fast::clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _183.w) * (1.0 / _71)) - texture1.sample(sampler1, _183.xy).x), 0.0, 1.0)));
    }
    else
    {
        _221 = float4(0.0);
    }
    out.out_var_SV_Target = float4(_221.xyz * _221.w, 1.0);
    return out;
}

