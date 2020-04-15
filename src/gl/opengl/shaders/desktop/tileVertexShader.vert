#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

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

uniform sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;

vec2 _48;

void main()
{
    vec2 _53 = in_var_POSITION.xy * 2.0;
    float _54 = _53.x;
    float _55 = floor(_54);
    float _56 = _53.y;
    float _57 = floor(_56);
    float _59 = min(2.0, _55 + 1.0);
    float _62 = _54 - _55;
    float _64 = _57 * 3.0;
    int _66 = int(_64 + _55);
    float _73 = min(2.0, _57 + 1.0) * 3.0;
    int _75 = int(_73 + _55);
    vec4 _84 = u_EveryObject.u_eyePositions[_66] + ((u_EveryObject.u_eyePositions[int(_64 + _59)] - u_EveryObject.u_eyePositions[_66]) * _62);
    vec4 _100 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy), 0.0);
    vec4 _123 = ((_84 + (((u_EveryObject.u_eyePositions[_75] + ((u_EveryObject.u_eyePositions[int(_73 + _59)] - u_EveryObject.u_eyePositions[_75]) * _62)) - _84) * (_56 - _57))) + ((vec4(u_EveryObject.u_tileNormal.xyz * (((_100.x * 255.0) + (_100.y * 65280.0)) - 32768.0), 1.0) * u_EveryObject.u_view) - (vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_view))) * u_EveryObject.u_projection;
    vec2 _134 = _48;
    _134.x = 1.0 + _123.w;
    gl_Position = _123;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = _134;
}

