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

uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;
uniform highp sampler2D SPIRV_Cross_Combinedtexture1sampler1;

in highp vec2 in_var_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _59 = texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0);
    highp float _65 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp float _71 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    highp vec4 _87 = vec4((in_var_TEXCOORD0.x * 2.0) - 1.0, ((1.0 - in_var_TEXCOORD0.y) * 2.0) - 1.0, (u_cameraPlaneParams.s_CameraFarPlane / _65) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _59.x * _71) - 1.0)), 1.0) * u_params.u_inverseProjection;
    highp vec4 _96 = vec4((_87 / vec4(_87.w)).xyz, 1.0);
    highp vec4 _97 = _96 * u_params.u_shadowMapVP[0];
    highp vec4 _100 = _96 * u_params.u_shadowMapVP[1];
    highp vec4 _103 = _96 * u_params.u_shadowMapVP[2];
    highp float _105 = _97.w;
    highp vec3 _109 = ((_97.xyz / vec3(_105)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _111 = _100.w;
    highp vec3 _115 = ((_100.xyz / vec3(_111)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _117 = _103.w;
    highp vec3 _121 = ((_103.xyz / vec3(_117)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    highp float _122 = _109.x;
    highp float _126 = _109.y;
    highp float _131 = _109.z;
    highp float _137 = _115.x;
    highp float _141 = _115.y;
    highp float _146 = _115.z;
    highp float _152 = _121.x;
    highp float _156 = _121.y;
    highp float _161 = _121.z;
    highp vec4 _183 = mix(mix(mix(vec4(0.0), vec4(_122 * 0.3333333432674407958984375, 1.0 - _126, _131, _105), vec4(float((((((_122 >= 0.0) && (_122 <= 1.0)) && (_126 >= 0.0)) && (_126 <= 1.0)) && (_131 >= 0.0)) && (_131 <= 1.0)))), vec4(0.3333333432674407958984375 + (_137 * 0.3333333432674407958984375), 1.0 - _141, _146, _111), vec4(float((((((_137 >= 0.0) && (_137 <= 1.0)) && (_141 >= 0.0)) && (_141 <= 1.0)) && (_146 >= 0.0)) && (_146 <= 1.0)))), vec4(0.666666686534881591796875 + (_152 * 0.3333333432674407958984375), 1.0 - _156, _161, _117), vec4(float((((((_152 >= 0.0) && (_152 <= 1.0)) && (_156 >= 0.0)) && (_156 <= 1.0)) && (_161 >= 0.0)) && (_161 <= 1.0))));
    highp vec4 _221;
    if ((length(_183.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_183.z * _65))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _221 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, vec4(clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _183.w) * (1.0 / _71)) - texture(SPIRV_Cross_Combinedtexture1sampler1, _183.xy).x), 0.0, 1.0)));
    }
    else
    {
        _221 = vec4(0.0);
    }
    out_var_SV_Target = vec4(_221.xyz * _221.w, 1.0);
}

