#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_params
{
    highp vec4 u_idOverride;
} u_params;

uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;

in highp vec2 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    bvec4 _49 = bvec4((u_params.u_idOverride.w == 0.0) || (abs(u_params.u_idOverride.w - texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0).w) <= 0.00150000001303851604461669921875));
    out_var_SV_Target = vec4(_49.x ? vec4(1.0, 1.0, 0.0, 0.0).x : vec4(0.0).x, _49.y ? vec4(1.0, 1.0, 0.0, 0.0).y : vec4(0.0).y, _49.z ? vec4(1.0, 1.0, 0.0, 0.0).z : vec4(0.0).z, _49.w ? vec4(1.0, 1.0, 0.0, 0.0).w : vec4(0.0).w);
}

