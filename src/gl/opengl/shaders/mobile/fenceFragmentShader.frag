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
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _46 = texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0);
    highp float _73;
    switch (0u)
    {
        case 0u:
        {
            if (varying_TEXCOORD1.y != 0.0)
            {
                _73 = varying_TEXCOORD1.x / varying_TEXCOORD1.y;
                break;
            }
            _73 = log2(varying_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    out_var_SV_Target0 = vec4(_46.xyz * varying_COLOR0.xyz, _46.w * varying_COLOR0.w);
    out_var_SV_Target1 = vec4(0.0, ((step(0.0, 1.0) * 2.0) - 1.0) * _73, 0.0, 0.0);
    gl_FragDepth = _73;
}

