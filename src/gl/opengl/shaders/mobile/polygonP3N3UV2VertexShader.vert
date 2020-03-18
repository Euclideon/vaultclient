#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    layout(row_major) mat4 u_worldMatrix;
    vec4 u_colour;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec2 in_var_TEXCOORD0;
out vec2 out_var_TEXCOORD0;
out vec3 out_var_NORMAL;
out vec4 out_var_COLOR0;
out vec2 out_var_TEXCOORD1;

vec2 _34;

void main()
{
    vec4 _54 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _59 = _34;
    _59.x = 1.0 + _54.w;
    gl_Position = _54;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_NORMAL = normalize((vec4(in_var_NORMAL, 0.0) * u_EveryObject.u_worldMatrix).xyz);
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD1 = _59;
}

