#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_Combinedu_texturesampler0;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec4 _30 = texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD0);
    float _33 = min(_30.w, in_var_COLOR0.w);
    out_var_SV_Target = vec4((_30.xyz * _33) + (in_var_COLOR0.xyz * (1.0 - _33)), 1.0);
}

