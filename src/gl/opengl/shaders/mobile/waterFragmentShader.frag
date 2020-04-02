#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_cameraPlaneParams
{
    highp float s_CameraNearPlane;
    highp float s_CameraFarPlane;
    highp float u_clipZNear;
    highp float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_EveryFrameFrag
{
    highp vec4 u_specularDir;
    layout(row_major) highp mat4 u_eyeNormalMatrix;
    layout(row_major) highp mat4 u_inverseViewMatrix;
} u_EveryFrameFrag;

uniform highp sampler2D SPIRV_Cross_CombinednormalMapTexturenormalMapSampler;
uniform highp sampler2D SPIRV_Cross_CombinedskyboxTextureskyboxSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec4 varying_COLOR0;
in highp vec4 varying_COLOR1;
in highp vec2 varying_TEXCOORD2;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec3 _85 = normalize(((texture(SPIRV_Cross_CombinednormalMapTexturenormalMapSampler, varying_TEXCOORD0).xyz * vec3(2.0)) - vec3(1.0)) + ((texture(SPIRV_Cross_CombinednormalMapTexturenormalMapSampler, varying_TEXCOORD1).xyz * vec3(2.0)) - vec3(1.0)));
    highp vec3 _87 = normalize(varying_COLOR0.xyz);
    highp vec3 _103 = normalize((vec4(_85, 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz);
    highp vec3 _105 = normalize(reflect(_87, _103));
    highp vec3 _129 = normalize((vec4(_105, 0.0) * u_EveryFrameFrag.u_inverseViewMatrix).xyz);
    out_var_SV_Target = vec4(mix(texture(SPIRV_Cross_CombinedskyboxTextureskyboxSampler, vec2(atan(_129.x, _129.y) + 3.1415927410125732421875, acos(_129.z)) * vec2(0.15915493667125701904296875, 0.3183098733425140380859375)).xyz, varying_COLOR1.xyz * mix(vec3(1.0, 1.0, 0.60000002384185791015625), vec3(0.3499999940395355224609375), vec3(pow(max(0.0, _85.z), 5.0))), vec3(((dot(_103, _87) * (-0.5)) + 0.5) * 0.75)) + vec3(pow(abs(dot(_105, normalize((vec4(normalize(u_EveryFrameFrag.u_specularDir.xyz), 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz))), 50.0) * 0.5), 1.0);
    gl_FragDepth = log2(varying_TEXCOORD2.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
}

