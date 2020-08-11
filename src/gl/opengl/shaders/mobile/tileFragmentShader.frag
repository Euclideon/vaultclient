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

uniform highp sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;
uniform highp sampler2D SPIRV_Cross_CombinednormalTexturenormalSampler;

in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
in highp vec2 varying_TEXCOORD3;
in highp vec3 varying_TEXCOORD4;
in highp vec3 varying_TEXCOORD5;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec3 _65 = (texture(SPIRV_Cross_CombinednormalTexturenormalSampler, varying_TEXCOORD3).xyz * vec3(2.0)) - vec3(1.0);
    highp vec3 _71 = _65;
    _71.y = _65.y * (-1.0);
    highp vec3 _73 = normalize(mat3(normalize(cross(varying_TEXCOORD5, varying_TEXCOORD4)), varying_TEXCOORD5, varying_TEXCOORD4) * _71);
    out_var_SV_Target0 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0).xyz * varying_COLOR0.xyz, varying_COLOR0.w);
    out_var_SV_Target1 = vec4(varying_TEXCOORD2.x, ((step(0.0, _73.z) * 2.0) - 1.0) * (((varying_TEXCOORD1.x / varying_TEXCOORD1.y) * (1.0 / (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear))) + (u_cameraPlaneParams.u_clipZNear * (-0.5))), _73.xy);
}

