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

vec2 _57;

void main()
{
    vec2 _61 = in_var_POSITION.xy * 1.0;
    float _63 = _61.x;
    float _64 = floor(_63);
    float _65 = _61.y;
    float _66 = floor(_65);
    float _68 = min(1.0, _64 + 1.0);
    float _73 = _66 * 2.0;
    int _75 = int(_73 + _64);
    int _79 = int(_73 + _68);
    float _82 = min(1.0, _66 + 1.0) * 2.0;
    int _84 = int(_82 + _64);
    int _88 = int(_82 + _68);
    vec4 _91 = vec4(_63 - _64);
    vec4 _94 = vec4(_65 - _66);
    vec4 _96 = normalize(mix(mix(u_EveryObject.u_worldNormals[_75], u_EveryObject.u_worldNormals[_79], _91), mix(u_EveryObject.u_worldNormals[_84], u_EveryObject.u_worldNormals[_88], _91), _94));
    vec3 _97 = _96.xyz;
    vec4 _109 = mix(mix(u_EveryObject.u_eyePositions[_75], u_EveryObject.u_eyePositions[_79], _91), mix(u_EveryObject.u_eyePositions[_84], u_EveryObject.u_eyePositions[_88], _91), _94);
    vec2 _122 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _154 = (vec4(mix(_109.xyz, (vec4(_97 * _109.w, 1.0) * u_EveryObject.u_view).xyz, vec3(u_EveryObject.u_objectInfo.z)), 1.0) + ((vec4(_96.xyz, 0.0) * u_EveryObject.u_view) * (textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _122, 0.0).x + (in_var_POSITION.z * u_EveryObject.u_objectInfo.y)))) * u_EveryObject.u_projection;
    float _165 = _154.w;
    float _171 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _165)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _165;
    vec4 _172 = _154;
    _172.z = _171;
    vec2 _184 = _57;
    _184.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _172;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    varying_TEXCOORD1 = vec2(_171, _165);
    varying_TEXCOORD2 = _184;
    varying_TEXCOORD3 = _122;
    varying_TEXCOORD4 = _97;
    varying_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_75], u_EveryObject.u_worldBitangents[_79], _91), mix(u_EveryObject.u_worldBitangents[_84], u_EveryObject.u_worldBitangents[_88], _91), _94)).xyz;
}

