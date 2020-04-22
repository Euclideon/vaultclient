#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec2 in_var_TEXCOORD2;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = ((vec4(0.0, 0.279009997844696044921875, 0.0, 0.0) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0)) + (vec4(1.0, 0.44198000431060791015625, 0.0, 0.0) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD1))) + (vec4(0.0, 0.279009997844696044921875, 0.0, 0.0) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD2));
}

