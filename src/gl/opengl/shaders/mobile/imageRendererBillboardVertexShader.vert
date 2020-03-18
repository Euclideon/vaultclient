#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 out_var_TEXCOORD0;
out vec4 out_var_COLOR0;
out vec2 out_var_TEXCOORD1;

vec2 _32;

void main()
{
    vec4 _38 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    float _44 = _38.w;
    vec2 _50 = _38.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _44)) * in_var_POSITION.xy);
    vec2 _55 = _32;
    _55.x = 1.0 + _44;
    gl_Position = vec4(_50.x, _50.y, _38.z, _38.w);
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD1 = _55;
}

