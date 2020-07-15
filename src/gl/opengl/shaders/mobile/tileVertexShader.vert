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

vec2 _59;

void main()
{
    vec2 _64 = in_var_POSITION.xy * 1.0;
    float _66 = _64.x;
    float _67 = floor(_66);
    float _68 = _64.y;
    float _69 = floor(_68);
    float _71 = min(1.0, _67 + 1.0);
    float _76 = _69 * 2.0;
    int _78 = int(_76 + _67);
    int _82 = int(_76 + _71);
    float _85 = min(1.0, _69 + 1.0) * 2.0;
    int _87 = int(_85 + _67);
    int _91 = int(_85 + _71);
    vec4 _94 = vec4(_66 - _67);
    vec4 _97 = vec4(_68 - _69);
    vec4 _99 = normalize(mix(mix(u_EveryObject.u_worldNormals[_78], u_EveryObject.u_worldNormals[_82], _94), mix(u_EveryObject.u_worldNormals[_87], u_EveryObject.u_worldNormals[_91], _94), _97));
    vec2 _125 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _129 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _125, 0.0);
    vec4 _145 = (mix(mix(u_EveryObject.u_eyePositions[_78], u_EveryObject.u_eyePositions[_82], _94), mix(u_EveryObject.u_eyePositions[_87], u_EveryObject.u_eyePositions[_91], _94), _97) + ((vec4(_99.xyz, 0.0) * u_EveryObject.u_view) * ((((_129.x * 255.0) + (_129.y * 65280.0)) - 32768.0) + (in_var_POSITION.z * u_EveryObject.u_objectInfo.y)))) * u_EveryObject.u_projection;
    float _156 = _145.w;
    float _162 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _156)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _156;
    vec4 _163 = _145;
    _163.z = _162;
    vec2 _175 = _59;
    _175.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _163;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    varying_TEXCOORD1 = vec2(_162, _156);
    varying_TEXCOORD2 = _175;
    varying_TEXCOORD3 = _125;
    varying_TEXCOORD4 = _99.xyz;
    varying_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_78], u_EveryObject.u_worldBitangents[_82], _94), mix(u_EveryObject.u_worldBitangents[_87], u_EveryObject.u_worldBitangents[_91], _94), _97)).xyz;
}

