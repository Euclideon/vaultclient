#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_Combinedu_texturesampler0;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec2 in_var_TEXCOORD2;
layout(location = 3) in vec2 in_var_TEXCOORD3;
layout(location = 4) in vec2 in_var_TEXCOORD4;
layout(location = 5) in vec4 in_var_COLOR0;
layout(location = 6) in vec4 in_var_COLOR1;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec4 _99;
    switch (0u)
    {
        default:
        {
            vec4 _47 = texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD0);
            float _48 = _47.w;
            float _49 = _47.x;
            if (_49 == 0.0)
            {
                _99 = vec4(in_var_COLOR0.xyz, (_48 * in_var_COLOR1.z) * in_var_COLOR0.w);
                break;
            }
            float _63 = 0.1500000059604644775390625 * in_var_COLOR0.w;
            _99 = vec4(in_var_COLOR0.xyz, max(in_var_COLOR1.w, ((((1.0 - _48) + (_63 * step(texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD1).x - _49, -9.9999997473787516355514526367188e-06))) + (_63 * step(texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD2).x - _49, -9.9999997473787516355514526367188e-06))) + (_63 * step(texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD3).x - _49, -9.9999997473787516355514526367188e-06))) + (_63 * step(texture(SPIRV_Cross_Combinedu_texturesampler0, in_var_TEXCOORD4).x - _49, -9.9999997473787516355514526367188e-06))) * in_var_COLOR0.w);
            break;
        }
    }
    out_var_SV_Target = _99;
}

