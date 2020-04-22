#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_params
{
    vec4 u_idOverride;
} u_params;

uniform sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    bvec4 _49 = bvec4((u_params.u_idOverride.w == 0.0) || (abs(u_params.u_idOverride.w - texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD0).w) <= 0.00150000001303851604461669921875));
    out_var_SV_Target = vec4(_49.x ? vec4(1.0, 1.0, 0.0, 0.0).x : vec4(0.0).x, _49.y ? vec4(1.0, 1.0, 0.0, 0.0).y : vec4(0.0).y, _49.z ? vec4(1.0, 1.0, 0.0, 0.0).z : vec4(0.0).z, _49.w ? vec4(1.0, 1.0, 0.0, 0.0).w : vec4(0.0).w);
}

