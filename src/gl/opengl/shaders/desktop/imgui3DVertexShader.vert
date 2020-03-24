#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec2 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;

void main()
{
    vec4 _35 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _43 = _35.xy + ((u_EveryObject.u_screenSize.xy * in_var_POSITION) * _35.w);
    gl_Position = vec4(_43.x, _43.y, _35.z, _35.w);
    out_var_COLOR0 = in_var_COLOR0;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
}

