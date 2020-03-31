#version 300 es

layout(std140) uniform type_u_params
{
    vec4 u_screenParams;
    vec4 u_saturation;
} u_params;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;
out vec2 varying_TEXCOORD3;
out vec2 varying_TEXCOORD4;
out vec2 varying_TEXCOORD5;
out float varying_TEXCOORD6;

void main()
{
    gl_Position = vec4(in_var_POSITION.xy, 0.0, 1.0);
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_TEXCOORD1 = in_var_TEXCOORD0 + u_params.u_screenParams.xy;
    varying_TEXCOORD2 = in_var_TEXCOORD0 - u_params.u_screenParams.xy;
    varying_TEXCOORD3 = in_var_TEXCOORD0 + vec2(u_params.u_screenParams.x, -u_params.u_screenParams.y);
    varying_TEXCOORD4 = in_var_TEXCOORD0 + vec2(-u_params.u_screenParams.x, u_params.u_screenParams.y);
    varying_TEXCOORD5 = u_params.u_screenParams.xy;
    varying_TEXCOORD6 = u_params.u_saturation.x;
}

