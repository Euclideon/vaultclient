#version 300 es

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    varying_TEXCOORD0 = in_var_TEXCOORD0;
}

