#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryFrame
{
    layout(row_major) mat4 ProjectionMatrix;
} u_EveryFrame;

layout(location = 0) in vec2 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;

void main()
{
    gl_Position = vec4(in_var_POSITION, 0.0, 1.0) * u_EveryFrame.ProjectionMatrix;
    out_var_COLOR0 = in_var_COLOR0;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
}

