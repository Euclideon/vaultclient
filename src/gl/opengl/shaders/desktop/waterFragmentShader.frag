#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_EveryFrameFrag
{
    vec4 u_specularDir;
    layout(row_major) mat4 u_eyeNormalMatrix;
    layout(row_major) mat4 u_inverseViewMatrix;
} u_EveryFrameFrag;

uniform sampler2D SPIRV_Cross_CombinednormalMapTexturenormalMapSampler;
uniform sampler2D SPIRV_Cross_CombinedskyboxTextureskyboxSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 3) in vec4 in_var_COLOR1;
layout(location = 4) in vec2 in_var_TEXCOORD2;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _84 = texture(SPIRV_Cross_CombinednormalMapTexturenormalMapSampler, in_var_TEXCOORD0);
    vec4 _89 = texture(SPIRV_Cross_CombinednormalMapTexturenormalMapSampler, in_var_TEXCOORD1);
    vec3 _94 = normalize(((_84.xyz * vec3(2.0)) - vec3(1.0)) + ((_89.xyz * vec3(2.0)) - vec3(1.0)));
    vec3 _96 = normalize(in_var_COLOR0.xyz);
    vec3 _112 = normalize((vec4(_94, 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz);
    vec3 _114 = normalize(reflect(_96, _112));
    vec3 _138 = normalize((vec4(_114, 0.0) * u_EveryFrameFrag.u_inverseViewMatrix).xyz);
    vec4 _148 = texture(SPIRV_Cross_CombinedskyboxTextureskyboxSampler, vec2(atan(_138.x, _138.y) + 3.1415927410125732421875, acos(_138.z)) * vec2(0.15915493667125701904296875, 0.3183098733425140380859375));
    float _175;
    switch (0u)
    {
        default:
        {
            if (in_var_TEXCOORD2.y != 0.0)
            {
                _175 = in_var_TEXCOORD2.x / in_var_TEXCOORD2.y;
                break;
            }
            _175 = log2(in_var_TEXCOORD2.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    out_var_SV_Target0 = (vec4(mix(_148.xyz, in_var_COLOR1.xyz * mix(vec3(1.0, 1.0, 0.60000002384185791015625), vec3(0.3499999940395355224609375), vec3(pow(max(0.0, _94.z), 5.0))), vec3(((dot(_112, _96) * (-0.5)) + 0.5) * 0.75)) + vec3(pow(abs(dot(_114, normalize((vec4(normalize(u_EveryFrameFrag.u_specularDir.xyz), 0.0) * u_EveryFrameFrag.u_eyeNormalMatrix).xyz))), 50.0) * 0.5), 1.0) * 0.300000011920928955078125) + vec4(0.20000000298023223876953125, 0.4000000059604644775390625, 0.699999988079071044921875, 1.0);
    out_var_SV_Target1 = vec4(0.0, ((step(0.0, 0.0) * 2.0) - 1.0) * _175, 0.0, 0.0);
    gl_FragDepth = _175;
}

