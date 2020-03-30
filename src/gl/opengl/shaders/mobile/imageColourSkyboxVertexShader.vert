#version 300 es

layout(std140) uniform type_u_EveryFrame
{
    vec4 u_tintColour;
    vec4 u_imageSize;
} u_EveryFrame;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec4 varying_COLOR0;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    varying_TEXCOORD0 = in_var_TEXCOORD0 / u_EveryFrame.u_imageSize.xy;
    varying_COLOR0 = u_EveryFrame.u_tintColour;
}

