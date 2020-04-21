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

in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

float _33;

void main()
{
    highp float _56 = log2(varying_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    highp vec4 _57 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0).xyz * varying_COLOR0.xyz, _33);
    _57.w = _56;
    out_var_SV_Target = _57;
    gl_FragDepth = _56;
}

