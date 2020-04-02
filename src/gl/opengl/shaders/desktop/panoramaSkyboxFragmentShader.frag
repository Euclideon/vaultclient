#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_EveryFrame
{
    layout(row_major) mat4 u_inverseViewProjection;
} u_EveryFrame;

uniform sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

layout(location = 0) in vec4 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec4 _40 = vec4(in_var_TEXCOORD0.xy, 1.0, 1.0) * u_EveryFrame.u_inverseViewProjection;
    vec3 _45 = normalize(_40.xyz / vec3(_40.w));
    out_var_SV_Target = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, vec2(atan(_45.x, _45.y) + 3.1415927410125732421875, acos(_45.z)) * vec2(0.15915493667125701904296875, 0.3183098733425140380859375));
}

