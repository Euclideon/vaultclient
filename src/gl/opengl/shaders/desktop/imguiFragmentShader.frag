#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedTextureTextureTextureSampler;

layout(location = 0) in vec4 in_var_COLOR0;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = in_var_COLOR0 * texture(SPIRV_Cross_CombinedTextureTextureTextureSampler, in_var_TEXCOORD0);
}

