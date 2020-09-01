#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _33 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, in_var_TEXCOORD0);
    float _36 = min(_33.w, in_var_COLOR0.w);
    out_var_SV_Target0 = vec4((_33.xyz * _36) + (in_var_COLOR0.xyz * (1.0 - _36)), 1.0);
    out_var_SV_Target1 = vec4(0.0);
}

