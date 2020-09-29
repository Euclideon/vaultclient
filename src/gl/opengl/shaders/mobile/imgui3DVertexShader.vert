#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec2 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;

void main()
{
    vec4 _44 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    float _49 = _44.w;
    vec2 _52 = _44.xy + ((u_EveryObject.u_screenSize.xy * in_var_POSITION) * _49);
    vec2 _65;
    switch (0u)
    {
        case 0u:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _65 = vec2(_44.z, _49);
                break;
            }
            _65 = vec2(1.0 + _49, 0.0);
            break;
        }
    }
    gl_Position = vec4(_52.x, _52.y, _44.z, _44.w);
    varying_COLOR0 = in_var_COLOR0;
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_TEXCOORD1 = _65;
    varying_TEXCOORD2 = vec2(u_EveryObject.u_screenSize.w);
}

