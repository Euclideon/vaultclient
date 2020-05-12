#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _36 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD0);
    float _37 = _36.x;
    out_var_SV_Target0 = vec4(texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD0).zyx, 1.0);
    out_var_SV_Target1 = vec4(0.0, 0.0, 0.0, _37);
    gl_FragDepth = _37;
}

