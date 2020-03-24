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

layout(location = 0) in vec2 in_var_POSITION;
out vec2 out_var_TEXCOORD0;
out vec2 out_var_TEXCOORD1;
out vec4 out_var_COLOR0;
out vec3 out_var_COLOR1;

void main()
{
    float _48 = u_EveryFrameVert.u_time.x * 0.0625;
    vec4 _63 = vec4(in_var_POSITION, 0.0, 1.0);
    gl_Position = _63 * u_EveryObject.u_worldViewProjectionMatrix;
    out_var_TEXCOORD0 = ((in_var_POSITION * u_EveryObject.u_colourAndSize.w) * vec2(0.25)) - vec2(_48);
    out_var_TEXCOORD1 = ((in_var_POSITION.yx * u_EveryObject.u_colourAndSize.w) * vec2(0.5)) - vec2(_48, u_EveryFrameVert.u_time.x * 0.046875);
    out_var_COLOR0 = _63 * u_EveryObject.u_worldViewMatrix;
    out_var_COLOR1 = u_EveryObject.u_colourAndSize.xyz;
}

