#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

uniform sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 3) in vec2 in_var_TEXCOORD1;
layout(location = 4) in vec2 in_var_TEXCOORD2;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

float _43;

void main()
{
    vec4 _54 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, in_var_TEXCOORD0) * in_var_COLOR0;
    float _73 = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    vec4 _83 = vec4(in_var_TEXCOORD2.x, ((step(0.0, in_var_NORMAL.z) * 2.0) - 1.0) * _73, in_var_NORMAL.x, _43);
    _83.w = in_var_TEXCOORD2.y;
    out_var_SV_Target0 = vec4(_54.xyz * ((dot(in_var_NORMAL, normalize(vec3(0.85000002384185791015625, 0.1500000059604644775390625, 0.5))) * 0.5) + 0.5), _54.w);
    out_var_SV_Target1 = _83;
    gl_FragDepth = _73;
}

