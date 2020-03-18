#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_TEXCOORD0;

void main()
{
    vec4 _21 = vec4(in_var_POSITION.xy, 0.0, 1.0);
    gl_Position = _21;
    out_var_TEXCOORD0 = _21;
}

