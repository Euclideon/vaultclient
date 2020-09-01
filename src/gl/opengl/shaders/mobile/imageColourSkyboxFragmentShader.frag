#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec4 varying_COLOR0;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _33 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, varying_TEXCOORD0);
    highp float _36 = min(_33.w, varying_COLOR0.w);
    out_var_SV_Target0 = vec4((_33.xyz * _36) + (varying_COLOR0.xyz * (1.0 - _36)), 1.0);
    out_var_SV_Target1 = vec4(0.0);
}

