#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryFrame
{
    vec4 u_stepSizeThickness;
    vec4 u_colour;
} u_EveryFrame;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec2 out_var_TEXCOORD1;
layout(location = 2) out vec2 out_var_TEXCOORD2;
layout(location = 3) out vec2 out_var_TEXCOORD3;
layout(location = 4) out vec2 out_var_TEXCOORD4;
layout(location = 5) out vec4 out_var_COLOR0;
layout(location = 6) out vec4 out_var_COLOR1;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_TEXCOORD1 = in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * vec2(-1.0));
    out_var_TEXCOORD2 = in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * vec2(1.0, -1.0));
    out_var_TEXCOORD3 = in_var_TEXCOORD0 + (u_EveryFrame.u_stepSizeThickness.xy * vec2(-1.0, 1.0));
    out_var_TEXCOORD4 = in_var_TEXCOORD0 + u_EveryFrame.u_stepSizeThickness.xy;
    out_var_COLOR0 = u_EveryFrame.u_colour;
    out_var_COLOR1 = u_EveryFrame.u_stepSizeThickness;
}

