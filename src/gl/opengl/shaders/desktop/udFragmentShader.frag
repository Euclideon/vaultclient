#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_Combinedtexture0sampler0;
uniform sampler2D SPIRV_Cross_Combinedtexture1sampler1;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = vec4(texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0).zyx, 1.0);
    gl_FragDepth = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD0).x;
}

