#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec3 u_sunDirection;
    float _padding;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
out vec3 varying_COLOR0;
out vec4 varying_COLOR1;
out vec3 varying_COLOR2;
out vec4 varying_COLOR3;
out vec2 varying_TEXCOORD0;

void main()
{
    vec4 _50 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _70;
    switch (0u)
    {
        case 0u:
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
    varying_COLOR0 = (in_var_NORMAL * 0.5) + vec3(0.5);
    varying_COLOR1 = u_EveryObject.u_colour;
    varying_COLOR2 = u_EveryObject.u_sunDirection;
    varying_COLOR3 = _50;
    varying_TEXCOORD0 = _70;
}

