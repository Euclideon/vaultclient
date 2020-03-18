#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_params
{
    vec4 u_idOverride;
} u_params;

uniform sampler2D SPIRV_Cross_Combinedtexture0sampler0;
uniform sampler2D SPIRV_Cross_Combinedtexture1sampler1;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec4 _42 = texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0);
    vec4 _46 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD0);
    float _51 = _42.w;
    vec4 _59;
    if ((u_params.u_idOverride.w == 0.0) || (abs(u_params.u_idOverride.w - _51) <= 0.00150000001303851604461669921875))
    {
        _59 = vec4(_51, 0.0, 0.0, 1.0);
    }
    else
    {
        _59 = vec4(0.0);
    }
    out_var_SV_Target = _59;
    gl_FragDepth = _46.x;
}

