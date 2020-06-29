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

uniform highp sampler2D SPIRV_Cross_CombinedTextureTextureTextureSampler;

in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _58 = vec4(varying_TEXCOORD2.x, ((step(0.0, 0.0) * 2.0) - 1.0) * (log2(varying_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))), 0.0, 0.0);
    _58.w = 1.0;
    out_var_SV_Target0 = varying_COLOR0 * texture(SPIRV_Cross_CombinedTextureTextureTextureSampler, varying_TEXCOORD0);
    out_var_SV_Target1 = _58;
}

