#version 300 es

layout(std140) uniform type_u_EveryFrameVert
{
    vec4 u_time;
} u_EveryFrameVert;

layout(std140) uniform type_u_EveryObject
{
    vec4 u_colourAndSize;
    layout(row_major) mat4 u_worldViewMatrix;
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec4 varying_COLOR0;
out vec3 varying_COLOR1;
out vec2 varying_TEXCOORD2;

void main()
{
    float _57 = u_EveryFrameVert.u_time.x * 0.0625;
    vec4 _73 = vec4(in_var_POSITION, 1.0);
    vec4 _79 = _73 * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _93;
    switch (0u)
    {
        case 0u:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _93 = vec2(_79.zw);
                break;
            }
            _93 = vec2(1.0 + _79.w, 0.0);
            break;
        }
    }
    gl_Position = _79;
    varying_TEXCOORD0 = ((in_var_TEXCOORD0 * u_EveryObject.u_colourAndSize.w) * vec2(0.25)) - vec2(_57);
    varying_TEXCOORD1 = ((in_var_TEXCOORD0.yx * u_EveryObject.u_colourAndSize.w) * vec2(0.5)) - vec2(_57, u_EveryFrameVert.u_time.x * 0.046875);
    varying_COLOR0 = _73 * u_EveryObject.u_worldViewMatrix;
    varying_COLOR1 = u_EveryObject.u_colourAndSize.xyz;
    varying_TEXCOORD2 = _93;
}

