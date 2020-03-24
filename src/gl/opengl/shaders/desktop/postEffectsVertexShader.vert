#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_params
{
    vec4 u_screenParams;
    vec4 u_saturation;
} u_params;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec2 out_var_TEXCOORD1;
layout(location = 2) out vec2 out_var_TEXCOORD2;
layout(location = 3) out vec2 out_var_TEXCOORD3;
layout(location = 4) out vec2 out_var_TEXCOORD4;
layout(location = 5) out vec2 out_var_TEXCOORD5;
layout(location = 6) out float out_var_TEXCOORD6;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_TEXCOORD1 = in_var_TEXCOORD0 + u_params.u_screenParams.xy;
    out_var_TEXCOORD2 = in_var_TEXCOORD0 - u_params.u_screenParams.xy;
    out_var_TEXCOORD3 = in_var_TEXCOORD0 + vec2(u_params.u_screenParams.x, -u_params.u_screenParams.y);
    out_var_TEXCOORD4 = in_var_TEXCOORD0 + vec2(-u_params.u_screenParams.x, u_params.u_screenParams.y);
    out_var_TEXCOORD5 = u_params.u_screenParams.xy;
    out_var_TEXCOORD6 = u_params.u_saturation.x;
}

