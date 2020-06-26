#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
in highp vec2 varying_TEXCOORD3;
in highp vec2 varying_TEXCOORD4;
in highp vec4 varying_COLOR0;
in highp vec4 varying_COLOR1;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _103;
    switch (0u)
    {
        case 0u:
        {
            highp vec4 _49 = texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0);
            highp float _50 = _49.x;
            if (_50 == 0.0)
            {
                _103 = vec4(varying_COLOR0.xyz, min(1.0, (_49.y * varying_COLOR1.z) * varying_COLOR0.w));
                break;
            }
            highp float _67 = 0.1500000059604644775390625 * varying_COLOR0.w;
            _103 = vec4(varying_COLOR0.xyz, max(varying_COLOR1.w, ((((1.0 - _49.y) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD1).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD2).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD3).x - _50, -9.9999997473787516355514526367188e-06))) + (_67 * step(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD4).x - _50, -9.9999997473787516355514526367188e-06))) * varying_COLOR0.w);
            break;
        }
    }
    out_var_SV_Target0 = _103;
    out_var_SV_Target1 = vec4(0.0);
}

