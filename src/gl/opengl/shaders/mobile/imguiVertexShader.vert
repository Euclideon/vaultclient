#version 300 es

layout(std140) uniform type_u_EveryFrame
{
    layout(row_major) mat4 ProjectionMatrix;
} u_EveryFrame;

layout(location = 0) in vec2 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;

void main()
{
    gl_Position = vec4(in_var_POSITION, 0.0, 1.0) * u_EveryFrame.ProjectionMatrix;
    varying_COLOR0 = in_var_COLOR0;
    varying_TEXCOORD0 = in_var_TEXCOORD0;
}

