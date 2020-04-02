#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_cameraPlaneParams
{
    highp float s_CameraNearPlane;
    highp float s_CameraFarPlane;
    highp float u_clipZNear;
    highp float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_params
{
    layout(row_major) highp mat4 u_shadowMapVP[3];
    layout(row_major) highp mat4 u_inverseProjection;
    highp vec4 u_visibleColour;
    highp vec4 u_notVisibleColour;
    highp vec4 u_viewDistance;
} u_params;

uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform highp sampler2D SPIRV_Cross_CombinedshadowMapAtlasTextureshadowMapAtlasSampler;

in highp vec4 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _61 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD1);
    highp float _67 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp float _73 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    highp vec4 _91 = vec4(varying_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / _67) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _61.x * _73) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_params.u_inverseProjection;
    highp vec4 _100 = vec4((_91 / vec4(_91.w)).xyz, 1.0);
    highp vec4 _101 = _100 * u_params.u_shadowMapVP[0];
    highp vec4 _104 = _100 * u_params.u_shadowMapVP[1];
    highp vec4 _107 = _100 * u_params.u_shadowMapVP[2];
    highp float _109 = _101.w;
    highp vec3 _113 = ((_101.xyz / vec3(_109)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _115 = _104.w;
    highp vec3 _119 = ((_104.xyz / vec3(_115)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _121 = _107.w;
    highp vec3 _125 = ((_107.xyz / vec3(_121)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _126 = _113.x;
    highp float _130 = _113.y;
    highp float _135 = _113.z;
    highp float _141 = _119.x;
    highp float _145 = _119.y;
    highp float _150 = _119.z;
    highp float _156 = _125.x;
    highp float _160 = _125.y;
    highp float _165 = _125.z;
    highp vec4 _187 = mix(mix(mix(vec4(0.0), vec4(_126 * 0.3333333432674407958984375, 1.0 - _130, _135, _109), vec4(float((((((_126 >= 0.0) && (_126 <= 1.0)) && (_130 >= 0.0)) && (_130 <= 1.0)) && (_135 >= 0.0)) && (_135 <= 1.0)))), vec4(0.3333333432674407958984375 + (_141 * 0.3333333432674407958984375), 1.0 - _145, _150, _115), vec4(float((((((_141 >= 0.0) && (_141 <= 1.0)) && (_145 >= 0.0)) && (_145 <= 1.0)) && (_150 >= 0.0)) && (_150 <= 1.0)))), vec4(0.666666686534881591796875 + (_156 * 0.3333333432674407958984375), 1.0 - _160, _165, _121), vec4(float((((((_156 >= 0.0) && (_156 <= 1.0)) && (_160 >= 0.0)) && (_160 <= 1.0)) && (_165 >= 0.0)) && (_165 <= 1.0))));
    highp vec4 _225;
    if ((length(_187.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_187.z * _67))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _225 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, vec4(clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _187.w) * (1.0 / _73)) - texture(SPIRV_Cross_CombinedshadowMapAtlasTextureshadowMapAtlasSampler, _187.xy).x), 0.0, 1.0)));
    }
    else
    {
        _225 = vec4(0.0);
    }
    out_var_SV_Target = vec4(_225.xyz * _225.w, 1.0);
}

