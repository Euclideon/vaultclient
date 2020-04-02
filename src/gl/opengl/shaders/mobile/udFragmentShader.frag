#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

in highp vec2 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = vec4(texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0).zyx, 1.0);
    gl_FragDepth = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0).x;
}

