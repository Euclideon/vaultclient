#version 300 es

layout(std140) uniform type_u_EveryObject
{
    vec4 u_colour;
    vec4 u_thickness;
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec4 in_var_POSITION;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 2) in vec4 in_var_COLOR1;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;

vec2 _33;

void main()
{
    vec4 _47 = vec4(in_var_POSITION.xyz, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    float _48 = _47.w;
    vec4 _50 = _47 / vec4(_48);
    vec2 _53 = (_50.xy * vec2(0.5)) + vec2(0.5);
    vec4 _58 = vec4(in_var_COLOR0.xyz, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _64 = ((_58.xy / vec2(_58.w)) * vec2(0.5)) + vec2(0.5);
    vec4 _70 = vec4(in_var_COLOR1.xyz, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _76 = ((_70.xy / vec2(_70.w)) * vec2(0.5)) + vec2(0.5);
    vec2 _80 = _53;
    _80.x = _53.x * u_EveryObject.u_thickness.y;
    vec4 _83 = vec4(_76.x, _76.y, _70.z, _70.w);
    _83.x = _76.x * u_EveryObject.u_thickness.y;
    vec4 _86 = vec4(_64.x, _64.y, _58.z, _58.w);
    _86.x = _64.x * u_EveryObject.u_thickness.y;
    vec2 _89 = normalize(_80 - _86.xy);
    vec2 _92 = normalize(_83.xy - _80);
    vec2 _94 = normalize(_89 + _92);
    vec2 _108 = vec2(-_94.y, _94.x) * ((u_EveryObject.u_thickness.x * 0.5) * (1.0 + pow(1.5 - (dot(_89, _92) * 0.5), 2.0)));
    vec2 _111 = _108;
    _111.x = _108.x / u_EveryObject.u_thickness.y;
    vec2 _119 = _33;
    _119.x = 1.0 + _48;
    gl_Position = _50 + vec4(_111 * in_var_POSITION.w, 0.0, 0.0);
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = _119;
}

