#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 0) out vec4 out_var_SV_Target0;

void main()
{
    out_var_SV_Target0 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, in_var_TEXCOORD0) * in_var_COLOR0;
}

