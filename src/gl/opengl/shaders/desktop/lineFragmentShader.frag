#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(location = 0) in vec4 in_var_COLOR0;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    float _37 = log2(in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out_var_SV_Target0 = in_var_COLOR0;
    out_var_SV_Target1 = vec4(0.0, ((step(0.0, 0.0) * 2.0) - 1.0) * _37, 0.0, 0.0);
    gl_FragDepth = _37;
}

