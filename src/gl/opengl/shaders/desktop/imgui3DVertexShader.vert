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
layout(location = 2) out vec2 out_var_TEXCOORD1;
layout(location = 3) out vec2 out_var_TEXCOORD2;

vec2 _33;

void main()
{
    vec4 _40 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    float _45 = _40.w;
    vec2 _48 = _40.xy + ((u_EveryObject.u_screenSize.xy * in_var_POSITION) * _45);
    vec2 _51 = _33;
    _51.x = 1.0 + _45;
    gl_Position = vec4(_48.x, _48.y, _40.z, _40.w);
    out_var_COLOR0 = in_var_COLOR0;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_TEXCOORD1 = _51;
    out_var_TEXCOORD2 = vec2(u_EveryObject.u_screenSize.w);
}

