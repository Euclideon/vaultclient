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
    vec4 u_eyePositions[9];
    vec4 u_eyeNormals[9];
    vec4 u_colour;
    vec4 u_objectInfo;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
} u_EveryObject;

uniform highp sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;
out vec2 varying_TEXCOORD3;

vec2 _56;

void main()
{
    vec2 _61 = in_var_POSITION.xy * 2.0;
    float _63 = _61.x;
    float _64 = floor(_63);
    float _65 = _61.y;
    float _66 = floor(_65);
    float _68 = min(2.0, _64 + 1.0);
    float _73 = _66 * 3.0;
    int _75 = int(_73 + _64);
    int _79 = int(_73 + _68);
    float _82 = min(2.0, _66 + 1.0) * 3.0;
    int _84 = int(_82 + _64);
    int _88 = int(_82 + _68);
    vec4 _91 = vec4(_63 - _64);
    vec4 _94 = vec4(_65 - _66);
    vec2 _113 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _117 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _113, 0.0);
    vec4 _128 = (mix(mix(u_EveryObject.u_eyePositions[_75], u_EveryObject.u_eyePositions[_79], _91), mix(u_EveryObject.u_eyePositions[_84], u_EveryObject.u_eyePositions[_88], _91), _94) + (mix(mix(u_EveryObject.u_eyeNormals[_75], u_EveryObject.u_eyeNormals[_79], _91), mix(u_EveryObject.u_eyeNormals[_84], u_EveryObject.u_eyeNormals[_88], _91), _94) * (((_117.x * 255.0) + (_117.y * 65280.0)) - 32768.0))) * u_EveryObject.u_projection;
    float _139 = _128.w;
    float _145 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _139)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _139;
    vec4 _146 = _128;
    _146.z = _145;
    vec2 _158 = _56;
    _158.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _146;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    varying_TEXCOORD1 = vec2(_145, _139);
    varying_TEXCOORD2 = _158;
    varying_TEXCOORD3 = _113;
}

