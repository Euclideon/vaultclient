#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec4 in_var_COLOR0;
layout(location = 3) in vec2 in_var_TEXCOORD1;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    out_var_SV_Target = in_var_COLOR0;
}

