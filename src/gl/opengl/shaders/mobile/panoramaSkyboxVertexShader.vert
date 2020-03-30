#version 300 es

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec4 varying_TEXCOORD0;

void main()
{
    vec4 _21 = vec4(in_var_POSITION.xy, 0.0, 1.0);
    gl_Position = _21;
    varying_TEXCOORD0 = _21;
}

