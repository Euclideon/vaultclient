#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

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
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;

vec2 _49;

void main()
{
    vec2 _65 = _49;
    _65.y = in_var_COLOR0.w;
    vec2 _97;
    vec3 _98;
    if (((u_EveryFrame.u_options >> 2) & 1) == 0)
    {
        vec2 _77 = _65;
        _77.x = (in_var_TEXCOORD0.y * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _97 = _77;
        _98 = vec3(u_EveryFrame.u_worldUp.x, u_EveryFrame.u_worldUp.y, u_EveryFrame.u_worldUp.z);
    }
    else
    {
        vec2 _92 = _65;
        _92.x = (in_var_TEXCOORD0.x * u_EveryFrame.u_textureRepeatScale) - u_EveryFrame.u_time;
        _97 = _92;
        _98 = vec3(in_var_COLOR0.xyz);
    }
    int _99 = u_EveryFrame.u_options & 15;
    vec3 _125;
    if (_99 == 0)
    {
        _125 = in_var_POSITION + ((_98 * u_EveryFrame.u_width) * in_var_COLOR0.w);
    }
    else
    {
        vec3 _124;
        if (_99 == 1)
        {
            _124 = in_var_POSITION - ((_98 * u_EveryFrame.u_width) * in_var_COLOR0.w);
        }
        else
        {
            _124 = in_var_POSITION + ((_98 * u_EveryFrame.u_width) * (in_var_COLOR0.w - 0.5));
        }
        _125 = _124;
    }
    vec4 _132 = vec4(_125, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _146;
    switch (0u)
    {
        default:
        {
            if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
            {
                _146 = vec2(_132.zw);
                break;
            }
            _146 = vec2(1.0 + _132.w, 0.0);
            break;
        }
    }
    gl_Position = _132;
    out_var_COLOR0 = mix(u_EveryFrame.u_bottomColour, u_EveryFrame.u_topColour, vec4(in_var_COLOR0.w));
    out_var_TEXCOORD0 = _97;
    out_var_TEXCOORD1 = _146;
}

