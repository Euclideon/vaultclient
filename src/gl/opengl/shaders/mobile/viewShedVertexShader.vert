#version 300 es

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec4 out_var_TEXCOORD0;
out vec2 out_var_TEXCOORD1;

void main()
{
    vec4 _24 = vec4(in_var_POSITION.xy, 0.0, 1.0);
    gl_Position = _24;
    out_var_TEXCOORD0 = _24;
    out_var_TEXCOORD1 = in_var_TEXCOORD0;
}

