#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;

vec2 _39;

void main()
{
    vec4 _49 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _65;
    switch (0u)
    {
        case 0u:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _65 = vec2(_49.zw);
                break;
            }
            _65 = vec2(1.0 + _49.w, 0.0);
            break;
        }
    }
    vec2 _68 = _39;
    _68.x = u_EveryObject.u_screenSize.w;
    gl_Position = _49;
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD1 = _65;
    varying_TEXCOORD2 = _68;
}

