#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryFrame
{
    vec4 u_stepSize;
} u_EveryFrame;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec2 out_var_TEXCOORD1;
layout(location = 2) out vec2 out_var_TEXCOORD2;

void main()
{
    vec2 _36 = u_EveryFrame.u_stepSize.xy * 1.41999995708465576171875;
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    out_var_TEXCOORD0 = in_var_TEXCOORD0 - _36;
    out_var_TEXCOORD1 = in_var_TEXCOORD0;
    out_var_TEXCOORD2 = in_var_TEXCOORD0 + _36;
}

