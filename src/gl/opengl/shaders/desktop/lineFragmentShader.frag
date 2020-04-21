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
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    float _34 = log2(in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    vec4 _35 = in_var_COLOR0;
    _35.w = _34;
    out_var_SV_Target = _35;
    gl_FragDepth = _34;
}

