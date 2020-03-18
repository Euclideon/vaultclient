#version 300 es

layout(std140) uniform type_u_EveryFrame
{
    vec4 u_bottomColour;
    vec4 u_topColour;
    float u_orientation;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
    float _padding1;
    vec2 _padding2;
} u_EveryFrame;

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
out vec4 out_var_COLOR0;
out vec2 out_var_TEXCOORD0;
out vec2 out_var_TEXCOORD1;

vec2 _41;

void main()
{
    vec4 _82 = vec4(in_var_POSITION + mix(vec3(0.0, 0.0, in_var_COLOR0.w) * u_EveryFrame.u_width, in_var_COLOR0.xyz, vec3(u_EveryFrame.u_orientation)), 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _85 = _41;
    _85.x = 1.0 + _82.w;
    gl_Position = _82;
    out_var_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, vec4(in_var_COLOR0.w));
    out_var_TEXCOORD0 = vec2((mix(in_var_TEXCOORD0.y, in_var_TEXCOORD0.x, u_EveryFrame.u_orientation) * u_EveryFrame.u_textureRepeatScale) - (u_EveryFrame.u_time * u_EveryFrame.u_textureScrollSpeed), in_var_COLOR0.w);
    out_var_TEXCOORD1 = _85;
}

