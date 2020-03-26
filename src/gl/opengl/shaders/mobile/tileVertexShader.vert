#version 300 es

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_projection;
    layout(row_major) mat4 u_view;
    vec4 u_eyePositions[9];
    vec4 u_colour;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
    vec4 u_tileNormal;
} u_EveryObject;

uniform highp sampler2D SPIRV_Cross_Combinedtexture1sampler1;

layout(location = 0) in vec3 in_var_POSITION;
out vec4 out_var_COLOR0;
out vec2 out_var_TEXCOORD0;
out vec2 out_var_TEXCOORD1;

vec2 _47;

void main()
{
    vec2 _51 = in_var_POSITION.xy * 2.0;
    float _52 = _51.x;
    float _53 = floor(_52);
    float _54 = _51.y;
    float _55 = floor(_54);
    float _57 = min(2.0, _53 + 1.0);
    float _60 = _52 - _53;
    float _62 = _55 * 3.0;
    int _64 = int(_62 + _53);
    float _71 = min(2.0, _55 + 1.0) * 3.0;
    int _73 = int(_71 + _53);
    vec4 _82 = u_EveryObject.u_eyePositions[_64] + ((u_EveryObject.u_eyePositions[int(_62 + _57)] - u_EveryObject.u_eyePositions[_64]) * _60);
    vec4 _117 = ((_82 + (((u_EveryObject.u_eyePositions[_73] + ((u_EveryObject.u_eyePositions[int(_71 + _57)] - u_EveryObject.u_eyePositions[_73]) * _60)) - _82) * (_54 - _55))) + ((vec4(u_EveryObject.u_tileNormal.xyz * (textureLod(SPIRV_Cross_Combinedtexture1sampler1, u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy), 0.0).x * 32768.0), 1.0) * u_EveryObject.u_view) - (vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_view))) * u_EveryObject.u_projection;
    vec2 _128 = _47;
    _128.x = 1.0 + _117.w;
    gl_Position = _117;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = _128;
}

