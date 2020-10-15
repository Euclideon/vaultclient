#version 300 es

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
    vec4 u_eyePositions[4];
    vec4 u_colour;
    vec4 u_objectInfo;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
    vec4 u_worldNormals[4];
    vec4 u_worldBitangents[4];
} u_EveryObject;

uniform highp sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;
out vec2 varying_TEXCOORD3;
out vec3 varying_TEXCOORD4;
out vec3 varying_TEXCOORD5;

vec2 _61;

void main()
{
    vec2 _65 = in_var_POSITION.xy * 1.0;
    float _67 = _65.x;
    float _68 = floor(_67);
    float _69 = _65.y;
    float _70 = floor(_69);
    float _72 = min(1.0, _68 + 1.0);
    float _77 = _70 * 2.0;
    int _79 = int(_77 + _68);
    int _83 = int(_77 + _72);
    float _86 = min(1.0, _70 + 1.0) * 2.0;
    int _88 = int(_86 + _68);
    int _92 = int(_86 + _72);
    vec4 _95 = vec4(_67 - _68);
    vec4 _98 = vec4(_69 - _70);
    vec4 _100 = normalize(mix(mix(u_EveryObject.u_worldNormals[_79], u_EveryObject.u_worldNormals[_83], _95), mix(u_EveryObject.u_worldNormals[_88], u_EveryObject.u_worldNormals[_92], _95), _98));
    vec3 _101 = _100.xyz;
    vec4 _113 = mix(mix(u_EveryObject.u_eyePositions[_79], u_EveryObject.u_eyePositions[_83], _95), mix(u_EveryObject.u_eyePositions[_88], u_EveryObject.u_eyePositions[_92], _95), _98);
    vec2 _126 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _158 = (vec4(mix(_113.xyz, (vec4(_101 * _113.w, 1.0) * u_EveryObject.u_view).xyz, vec3(u_EveryObject.u_objectInfo.z)), 1.0) + ((vec4(_100.xyz, 0.0) * u_EveryObject.u_view) * (textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _126, 0.0).x + (in_var_POSITION.z * u_EveryObject.u_objectInfo.y)))) * u_EveryObject.u_projection;
    float _184;
    switch (0u)
    {
        case 0u:
        {
            if (u_EveryObject.u_projection[3u].y != 0.0)
            {
                float _176 = _158.w;
                _184 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _176)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _176;
                break;
            }
            _184 = _158.z;
            break;
        }
    }
    vec4 _185 = _158;
    _185.z = _184;
    vec2 _198 = _61;
    _198.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _185;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    varying_TEXCOORD1 = vec2(_184, _158.w);
    varying_TEXCOORD2 = _198;
    varying_TEXCOORD3 = _126;
    varying_TEXCOORD4 = _101;
    varying_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_79], u_EveryObject.u_worldBitangents[_83], _95), mix(u_EveryObject.u_worldBitangents[_88], u_EveryObject.u_worldBitangents[_92], _95), _98)).xyz;
}

