#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_EveryFrame
{
    layout(row_major) highp mat4 u_inverseViewProjection;
} u_EveryFrame;

uniform highp sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

in highp vec4 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _43 = vec4(varying_TEXCOORD0.xy, 1.0, 1.0) * u_EveryFrame.u_inverseViewProjection;
    highp vec3 _48 = normalize(_43.xyz / vec3(_43.w));
    out_var_SV_Target0 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, vec2(atan(_48.x, _48.y) + 3.1415927410125732421875, acos(_48.z)) * vec2(0.15915493667125701904296875, 0.3183098733425140380859375));
    out_var_SV_Target1 = vec4(0.0);
}

