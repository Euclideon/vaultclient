#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

layout(location = 0) in vec4 in_var_COLOR0;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec2 in_var_TEXCOORD1;
layout(location = 0) out vec4 out_var_SV_Target;

float _23;

void main()
{
    vec4 _42 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0).xyz * in_var_COLOR0.xyz, _23);
    _42.w = in_var_TEXCOORD1.x / in_var_TEXCOORD1.y;
    out_var_SV_Target = _42;
}

