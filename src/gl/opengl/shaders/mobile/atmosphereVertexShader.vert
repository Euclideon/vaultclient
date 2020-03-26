#version 300 es

layout(std140) uniform type_u_vertParams
{
    layout(row_major) mat4 u_modelFromView;
    layout(row_major) mat4 u_viewFromClip;
} u_vertParams;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec3 varying_TEXCOORD1;

void main()
{
    vec4 _34 = vec4(in_var_POSITION, 1.0);
    gl_Position = _34;
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_TEXCOORD1 = (vec4((_34 * u_vertParams.u_viewFromClip).xyz, 0.0) * u_vertParams.u_modelFromView).xyz;
}

