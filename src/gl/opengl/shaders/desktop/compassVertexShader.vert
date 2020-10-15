#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec3 u_sunDirection;
    float _padding;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 0) out vec3 out_var_COLOR0;
layout(location = 1) out vec4 out_var_COLOR1;
layout(location = 2) out vec3 out_var_COLOR2;
layout(location = 3) out vec4 out_var_COLOR3;
layout(location = 4) out vec2 out_var_TEXCOORD0;

void main()
{
    vec4 _50 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _70;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _70 = vec2(_50.zw);
                break;
            }
            _70 = vec2(1.0 + _50.w, 0.0);
            break;
        }
    }
    gl_Position = _50;
    out_var_COLOR0 = (in_var_NORMAL * 0.5) + vec3(0.5);
    out_var_COLOR1 = u_EveryObject.u_colour;
    out_var_COLOR2 = u_EveryObject.u_sunDirection;
    out_var_COLOR3 = _50;
    out_var_TEXCOORD0 = _70;
}

