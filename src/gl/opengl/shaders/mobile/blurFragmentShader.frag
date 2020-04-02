#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = ((vec4(0.0, 0.0, 0.0, 0.279009997844696044921875) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD0)) + (vec4(1.0, 1.0, 1.0, 0.44198000431060791015625) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD1))) + (vec4(0.0, 0.0, 0.0, 0.279009997844696044921875) * texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, varying_TEXCOORD2));
}

