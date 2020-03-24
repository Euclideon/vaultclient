#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_projection;
    vec4 u_eyePositions[9];
    vec4 u_colour;
    vec4 u_uvOffsetScale;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;

vec2 _31;

void main()
{
    vec4 _40 = u_EveryObject.u_eyePositions[int(in_var_POSITION.z)] * u_EveryObject.u_projection;
    vec2 _52 = _31;
    _52.x = 1.0 + _40.w;
    gl_Position = _40;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = _52;
}

