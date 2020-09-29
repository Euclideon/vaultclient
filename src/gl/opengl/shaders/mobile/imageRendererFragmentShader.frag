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
in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _47 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, varying_TEXCOORD0);
    highp float _65;
    switch (0u)
    {
        case 0u:
        {
            if (varying_TEXCOORD1.y != 0.0)
            {
                _65 = varying_TEXCOORD1.x / varying_TEXCOORD1.y;
                break;
            }
            _65 = log2(varying_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    highp vec4 _72 = vec4(varying_TEXCOORD2.x, ((step(0.0, 0.0) * 2.0) - 1.0) * _65, 0.0, 0.0);
    _72.w = 1.0;
    out_var_SV_Target0 = _47 * varying_COLOR0;
    out_var_SV_Target1 = _72;
    gl_FragDepth = _65;
}

