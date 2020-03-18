#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_Combinedu_texturesampler0;

in highp vec2 in_var_TEXCOORD0;
in highp vec4 in_var_COLOR0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _30 = texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD0);
    highp float _33 = min(_30.w, in_var_COLOR0.w);
    out_var_SV_Target = vec4((_30.xyz * _33) + (in_var_COLOR0.xyz * (1.0 - _33)), 1.0);
}

