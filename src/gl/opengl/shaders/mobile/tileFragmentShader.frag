#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

float _23;

void main()
{
    highp vec4 _42 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0).xyz * varying_COLOR0.xyz, _23);
    _42.w = varying_TEXCOORD1.x / varying_TEXCOORD1.y;
    out_var_SV_Target = _42;
}

