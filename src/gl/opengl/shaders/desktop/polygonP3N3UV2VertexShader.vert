#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    layout(row_major) mat4 u_worldMatrix;
    vec4 u_colour;
    vec4 u_objectInfo;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec3 out_var_NORMAL;
layout(location = 2) out vec4 out_var_COLOR0;
layout(location = 3) out vec2 out_var_TEXCOORD1;
layout(location = 4) out vec2 out_var_TEXCOORD2;

void main()
{
    vec4 _61 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _77;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _77 = vec2(_61.zw);
                break;
            }
            _77 = vec2(1.0 + _61.w, 0.0);
            break;
        }
    }
    gl_Position = _61;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_NORMAL = normalize((vec4(in_var_NORMAL, 0.0) * u_EveryObject.u_worldMatrix).xyz);
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD1 = _77;
    out_var_TEXCOORD2 = u_EveryObject.u_objectInfo.xy;
}

