#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

uniform sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;
uniform sampler2D SPIRV_Cross_CombinednormalTexturenormalSampler;

layout(location = 0) in vec4 in_var_COLOR0;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec2 in_var_TEXCOORD1;
layout(location = 3) in vec2 in_var_TEXCOORD2;
layout(location = 4) in vec2 in_var_TEXCOORD3;
layout(location = 5) in vec3 in_var_TEXCOORD4;
layout(location = 6) in vec3 in_var_TEXCOORD5;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec3 _65 = (texture(SPIRV_Cross_CombinednormalTexturenormalSampler, in_var_TEXCOORD3).xyz * vec3(2.0)) - vec3(1.0);
    vec3 _71 = _65;
    _71.y = _65.y * (-1.0);
    vec3 _73 = normalize(mat3(normalize(cross(in_var_TEXCOORD5, in_var_TEXCOORD4)), in_var_TEXCOORD5, in_var_TEXCOORD4) * _71);
    out_var_SV_Target0 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0).xyz * in_var_COLOR0.xyz, in_var_COLOR0.w);
    out_var_SV_Target1 = vec4(in_var_TEXCOORD2.x, ((step(0.0, _73.z) * 2.0) - 1.0) * (((in_var_TEXCOORD1.x / in_var_TEXCOORD1.y) * (1.0 / (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear))) + (u_cameraPlaneParams.u_clipZNear * (-0.5))), _73.xy);
}

