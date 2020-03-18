#version 300 es

layout(std140) uniform type_u_params
{
    layout(row_major) mat4 u_screenProjection;
    vec4 u_screenParams;
    vec4 u_saturation;
} u_params;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 out_var_TEXCOORD0;
out vec2 out_var_TEXCOORD1;
out vec2 out_var_TEXCOORD2;
out vec2 out_var_TEXCOORD3;
out vec2 out_var_TEXCOORD4;
out vec2 out_var_TEXCOORD5;
out float out_var_TEXCOORD6;

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

