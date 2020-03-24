#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec3 u_sunDirection;
    float _padding;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
out vec3 out_var_COLOR0;
out vec4 out_var_COLOR1;
out vec3 out_var_COLOR2;
out vec4 out_var_COLOR3;
out vec2 out_var_TEXCOORD0;

vec2 _34;

void main()
{
    vec4 _44 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _53 = _34;
    _53.x = 1.0 + _44.w;
    gl_Position = _44;
    out_var_COLOR0 = (in_var_NORMAL * 0.5) + vec3(0.5);
    out_var_COLOR1 = u_EveryObject.u_colour;
    out_var_COLOR2 = u_EveryObject.u_sunDirection;
    out_var_COLOR3 = _44;
    out_var_TEXCOORD0 = _53;
}

