#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;

vec2 _39;

void main()
{
    vec4 _45 = vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    float _51 = _45.w;
    vec2 _57 = _45.xy + ((u_EveryObject.u_screenSize.xy * (u_EveryObject.u_screenSize.z * _51)) * in_var_POSITION.xy);
    vec2 _72;
    switch (0u)
    {
        case 0u:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _72 = vec2(_45.z, _51);
                break;
            }
            _72 = vec2(1.0 + _51, 0.0);
            break;
        }
    }
    vec2 _75 = _39;
    _75.x = u_EveryObject.u_screenSize.w;
    gl_Position = vec4(_57.x, _57.y, _45.z, _45.w);
    varying_TEXCOORD0 = in_var_TEXCOORD0;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD1 = _72;
    varying_TEXCOORD2 = _75;
}

