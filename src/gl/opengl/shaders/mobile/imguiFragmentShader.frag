#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;

in highp vec4 in_var_COLOR0;
in highp vec2 in_var_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = in_var_COLOR0 * texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0);
}

