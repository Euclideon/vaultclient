#version 300 es

layout(std140) uniform type_u_EveryFrame
{
    vec4 u_bottomColour;
    vec4 u_topColour;
    vec4 u_worldUp;
    int u_options;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
    float _padding1;
    float _padding2;
    float _padding3;
} u_EveryFrame;

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec4 in_var_COLOR0;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;

vec2 _44;

void main()
{
    vec2 _60 = _44;
    _60.y = in_var_COLOR0.w;
    vec2 _92;
    vec3 _93;
    if (((u_EveryFrame.u_options >> 2) & 1) == 0)
    {
        vec2 _72 = _60;
        _72.x = (in_var_TEXCOORD0.y * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _92 = _72;
        _93 = vec3(u_EveryFrame.u_worldUp.x, u_EveryFrame.u_worldUp.y, u_EveryFrame.u_worldUp.z);
    }
    else
    {
        vec2 _87 = _60;
        _87.x = (in_var_TEXCOORD0.x * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _92 = _87;
        _93 = vec3(in_var_COLOR0.xyz);
    }
    int _94 = u_EveryFrame.u_options & 15;
    vec3 _120;
    if (_94 == 0)
    {
        _120 = in_var_POSITION + ((_93 * u_EveryFrame.u_width) * in_var_COLOR0.w);
    }
    else
    {
        vec3 _119;
        if (_94 == 1)
        {
            _119 = in_var_POSITION - ((_93 * u_EveryFrame.u_width) * in_var_COLOR0.w);
        }
        else
        {
            _119 = in_var_POSITION + ((_93 * u_EveryFrame.u_width) * (in_var_COLOR0.w - 0.5));
        }
        _120 = _119;
    }
    vec4 _127 = vec4(_120, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _130 = _44;
    _130.x = 1.0 + _127.w;
    gl_Position = _127;
    varying_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, vec4(in_var_COLOR0.w));
    varying_TEXCOORD0 = _92;
    varying_TEXCOORD1 = _130;
}

