#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec2 in_var_TEXCOORD2;
layout(location = 3) in vec2 in_var_TEXCOORD3;
layout(location = 4) in vec2 in_var_TEXCOORD4;
layout(location = 5) in vec4 in_var_COLOR0;
layout(location = 6) in vec4 in_var_COLOR1;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _103;
    switch (0u)
    {
        default:
        {
            vec4 _49 = texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0);
            float _50 = _49.x;
            if (_50 == 0.0)
            {
                _103 = vec4(in_var_COLOR0.xyz, min(1.0, (_49.y * in_var_COLOR1.z) * in_var_COLOR0.w));
                break;
            }
            float _67 = 0.1500000059604644775390625 * in_var_COLOR0.w;
            _103 = vec4(in_var_COLOR0.xyz, max(in_var_COLOR1.w, ((((1.0 - _49.y) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD1).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD2).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD3).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD4).x - _50, -9.9999997473787516355514526367188e-06))) * in_var_COLOR0.w);
            break;
        }
    }
    out_var_SV_Target0 = _103;
    out_var_SV_Target1 = vec4(0.0);
}

