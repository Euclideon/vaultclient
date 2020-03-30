#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_params
{
    highp vec4 u_idOverride;
} u_params;

uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;
uniform highp sampler2D SPIRV_Cross_Combinedtexture1sampler1;

in highp vec2 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _42 = texture(SPIRV_Cross_Combinedtexture0sampler0, varying_TEXCOORD0);
    highp vec4 _46 = texture(SPIRV_Cross_Combinedtexture1sampler1, varying_TEXCOORD0);
    highp float _51 = _42.w;
    highp vec4 _59;
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

