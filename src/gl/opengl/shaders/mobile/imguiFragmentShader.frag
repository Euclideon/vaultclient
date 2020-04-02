#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedTextureTextureTextureSampler;

in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = varying_COLOR0 * texture(SPIRV_Cross_CombinedTextureTextureTextureSampler, varying_TEXCOORD0);
}

