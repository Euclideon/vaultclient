#version 300 es
precision mediump float;
precision highp int;

in highp vec2 varying_TEXCOORD0;
in highp vec3 varying_NORMAL;
in highp vec4 varying_COLOR0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target0;

void main()
{
    out_var_SV_Target0 = varying_COLOR0;
}

