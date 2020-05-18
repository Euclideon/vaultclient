#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec4 varying_COLOR0;

void main()
{
    vec4 _36 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _48 = _36.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _36.w)) * in_var_POSITION.xy);
    gl_Position = vec4(_48.x, _48.y, _36.z, _36.w);
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_COLOR0 = u_EveryObject.u_colour;
}

