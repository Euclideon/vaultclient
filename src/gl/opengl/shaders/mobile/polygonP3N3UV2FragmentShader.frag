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

uniform highp sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec3 varying_NORMAL;
in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _51 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, varying_TEXCOORD0) * varying_COLOR0;
    highp float _70 = log2(varying_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    highp float _74 = 1.0 - varying_NORMAL.z;
    out_var_SV_Target0 = vec4(_51.xyz * ((dot(varying_NORMAL, normalize(vec3(0.85000002384185791015625, 0.1500000059604644775390625, 0.5))) * 0.5) + 0.5), _51.w);
    out_var_SV_Target1 = vec4(varying_NORMAL.x / _74, varying_NORMAL.y / _74, varying_TEXCOORD2.x, _70);
    gl_FragDepth = _70;
}

