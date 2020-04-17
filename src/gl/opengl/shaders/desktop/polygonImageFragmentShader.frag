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
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, in_var_TEXCOORD0) * in_var_COLOR0;
    gl_FragDepth = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
}

