#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_projection;
    layout(row_major) mat4 u_view;
    vec4 u_eyePositions[9];
    vec4 u_colour;
    vec4 u_objectInfo;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
    vec4 u_worldNormals[9];
    vec4 u_worldBitangents[9];
} u_EveryObject;

uniform sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;
layout(location = 3) out vec2 out_var_TEXCOORD2;
layout(location = 4) out vec2 out_var_TEXCOORD3;
layout(location = 5) out vec3 out_var_TEXCOORD4;
layout(location = 6) out vec3 out_var_TEXCOORD5;

vec2 _60;

void main()
{
    vec2 _65 = in_var_POSITION.xy * 2.0;
    float _67 = _65.x;
    float _68 = floor(_67);
    float _69 = _65.y;
    float _70 = floor(_69);
    float _72 = min(2.0, _68 + 1.0);
    float _77 = _70 * 3.0;
    int _79 = int(_77 + _68);
    int _83 = int(_77 + _72);
    float _86 = min(2.0, _70 + 1.0) * 3.0;
    int _88 = int(_86 + _68);
    int _92 = int(_86 + _72);
    vec4 _95 = vec4(_67 - _68);
    vec4 _98 = vec4(_69 - _70);
    vec4 _100 = normalize(mix(mix(u_EveryObject.u_worldNormals[_79], u_EveryObject.u_worldNormals[_83], _95), mix(u_EveryObject.u_worldNormals[_88], u_EveryObject.u_worldNormals[_92], _95), _98));
    vec2 _126 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _130 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _126, 0.0);
    vec4 _146 = (mix(mix(u_EveryObject.u_eyePositions[_79], u_EveryObject.u_eyePositions[_83], _95), mix(u_EveryObject.u_eyePositions[_88], u_EveryObject.u_eyePositions[_92], _95), _98) + ((vec4(_100.xyz, 0.0) * u_EveryObject.u_view) * ((((_130.x * 255.0) + (_130.y * 65280.0)) - 32768.0) + (in_var_POSITION.z * u_EveryObject.u_objectInfo.y)))) * u_EveryObject.u_projection;
    float _157 = _146.w;
    float _163 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _157)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _157;
    vec4 _164 = _146;
    _164.z = _163;
    vec2 _176 = _60;
    _176.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _164;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = vec2(_163, _157);
    out_var_TEXCOORD2 = _176;
    out_var_TEXCOORD3 = _126;
    out_var_TEXCOORD4 = _100.xyz;
    out_var_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_79], u_EveryObject.u_worldBitangents[_83], _95), mix(u_EveryObject.u_worldBitangents[_88], u_EveryObject.u_worldBitangents[_92], _95), _98)).xyz;
}

