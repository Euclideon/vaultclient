#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

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
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec2 out_var_TEXCOORD1;
layout(location = 2) out vec4 out_var_COLOR0;
layout(location = 3) out vec3 out_var_COLOR1;
layout(location = 4) out vec2 out_var_TEXCOORD2;

vec2 _42;

void main()
{
    float _52 = u_EveryFrameVert.u_time.x * 0.0625;
    vec4 _68 = vec4(in_var_POSITION, 1.0);
    vec4 _74 = _68 * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _77 = _42;
    _77.x = 1.0 + _74.w;
    gl_Position = _74;
    out_var_TEXCOORD0 = ((in_var_TEXCOORD0 * u_EveryObject.u_colourAndSize.w) * vec2(0.25)) - vec2(_52);
    out_var_TEXCOORD1 = ((in_var_TEXCOORD0.yx * u_EveryObject.u_colourAndSize.w) * vec2(0.5)) - vec2(_52, u_EveryFrameVert.u_time.x * 0.046875);
    out_var_COLOR0 = _68 * u_EveryObject.u_worldViewMatrix;
    out_var_COLOR1 = u_EveryObject.u_colourAndSize.xyz;
    out_var_TEXCOORD2 = _77;
}

