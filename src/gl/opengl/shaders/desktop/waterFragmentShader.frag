#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_EveryFrameFrag
{
    vec4 u_specularDir;
    layout(row_major) mat4 u_eyeNormalMatrix;
    layout(row_major) mat4 u_inverseViewMatrix;
} u_EveryFrameFrag;

uniform sampler2D SPIRV_Cross_Combinedu_normalMapsampler0;
uniform sampler2D SPIRV_Cross_Combinedu_skyboxsampler1;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 3) in vec4 in_var_COLOR1;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec3 _77 = normalize(((texture(SPIRV_Cross_Combinedu_normalMapsampler0, in_var_TEXCOORD0).xyz * vec3(2.0)) - vec3(1.0)) + ((texture(SPIRV_Cross_Combinedu_normalMapsampler0, in_var_TEXCOORD1).xyz * vec3(2.0)) - vec3(1.0)));
    vec3 _79 = normalize(in_var_COLOR0.xyz);
    vec3 _95 = normalize((vec4(_77, 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz);
    vec3 _97 = normalize(reflect(_79, _95));
    vec3 _121 = normalize((vec4(_97, 0.0) * u_EveryFrameFrag.u_inverseViewMatrix).xyz);
    out_var_SV_Target = vec4(mix(texture(SPIRV_Cross_Combinedu_skyboxsampler1, vec2(atan(_121.x, _121.y) + 3.1415927410125732421875, acos(_121.z)) * vec2(0.15915493667125701904296875, 0.3183098733425140380859375)).xyz, in_var_COLOR1.xyz * mix(vec3(1.0, 1.0, 0.60000002384185791015625), vec3(0.3499999940395355224609375), vec3(pow(max(0.0, _77.z), 5.0))), vec3(((dot(_95, _79) * (-0.5)) + 0.5) * 0.75)) + vec3(pow(abs(dot(_97, normalize((vec4(normalize(u_EveryFrameFrag.u_specularDir.xyz), 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz))), 50.0) * 0.5), 1.0);
}

