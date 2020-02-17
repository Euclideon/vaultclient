#version 300 es

layout(std140) uniform type_u_EveryFrameVert
{
    vec4 u_time;
} u_EveryFrameVert;

layout(std140) uniform type_u_EveryObject
{
    vec4 u_colourAndSize;
    layout(row_major) mat4 u_worldViewMatrix;
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec4 varying_COLOR0;
out vec3 varying_COLOR1;
out vec2 varying_TEXCOORD2;

vec2 _42;

void main()
{
    float _52 = u_EveryFrameVert.u_time.x * 0.0625;
    vec4 _68 = vec4(in_var_POSITION, 1.0);
    vec4 _74 = _68 * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _77 = _42;
    _77.x = 1.0 + _74.w;
    gl_Position = _74;
    varying_TEXCOORD0 = ((in_var_TEXCOORD0 * u_EveryObject.u_colourAndSize.w) * vec2(0.25)) - vec2(_52);
    varying_TEXCOORD1 = ((in_var_TEXCOORD0.yx * u_EveryObject.u_colourAndSize.w) * vec2(0.5)) - vec2(_52, u_EveryFrameVert.u_time.x * 0.046875);
    varying_COLOR0 = _68 * u_EveryObject.u_worldViewMatrix;
    varying_COLOR1 = u_EveryObject.u_colourAndSize.xyz;
    varying_TEXCOORD2 = _77;
}

