#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD1;

vec2 _29;

void main()
{
    vec4 _39 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _44 = _29;
    _44.x = 1.0 + _39.w;
    gl_Position = _39;
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD1 = _44;
}

