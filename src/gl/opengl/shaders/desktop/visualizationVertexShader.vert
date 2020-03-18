#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_vertParams
{
    vec4 u_outlineStepSize;
} u_vertParams;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_TEXCOORD0;
layout(location = 1) out vec2 out_var_TEXCOORD1;
layout(location = 2) out vec2 out_var_TEXCOORD2;
layout(location = 3) out vec2 out_var_TEXCOORD3;
layout(location = 4) out vec2 out_var_TEXCOORD4;
layout(location = 5) out vec2 out_var_TEXCOORD5;

void main()
{
    vec4 _34 = vec4(in_var_POSITION.xy, 0.0, 1.0);
    vec3 _39 = vec3(u_vertParams.u_outlineStepSize.xy, 0.0);
    vec2 _40 = _39.xz;
    vec2 _43 = _39.zy;
    gl_Position = _34;
    out_var_TEXCOORD0 = _34;
    out_var_TEXCOORD1 = in_var_TEXCOORD0;
    out_var_TEXCOORD2 = in_var_TEXCOORD0 + _40;
    out_var_TEXCOORD3 = in_var_TEXCOORD0 - _40;
    out_var_TEXCOORD4 = in_var_TEXCOORD0 + _43;
    out_var_TEXCOORD5 = in_var_TEXCOORD0 - _43;
}

