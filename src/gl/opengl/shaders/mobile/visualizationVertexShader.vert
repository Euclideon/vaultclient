#version 300 es

layout(std140) uniform type_u_vertParams
{
    vec4 u_outlineStepSize;
} u_vertParams;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec4 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;
out vec2 varying_TEXCOORD3;
out vec2 varying_TEXCOORD4;
out vec2 varying_TEXCOORD5;

void main()
{
    vec4 _34 = vec4(in_var_POSITION.xy, 0.0, 1.0);
    vec3 _39 = vec3(u_vertParams.u_outlineStepSize.xy, 0.0);
    vec2 _40 = _39.xz;
    vec2 _43 = _39.zy;
    gl_Position = _34;
    varying_TEXCOORD0 = _34;
    varying_TEXCOORD1 = in_var_TEXCOORD0;
    varying_TEXCOORD2 = in_var_TEXCOORD0 + _40;
    varying_TEXCOORD3 = in_var_TEXCOORD0 - _40;
    varying_TEXCOORD4 = in_var_TEXCOORD0 + _43;
    varying_TEXCOORD5 = in_var_TEXCOORD0 - _43;
}

