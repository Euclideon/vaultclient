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
    vec4 u_colour;
    vec4 u_objectInfo;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
    vec4 u_tileNormal;
} u_EveryObject;

uniform highp sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;
out vec2 varying_TEXCOORD1;
out vec2 varying_TEXCOORD2;

vec2 _55;

void main()
{
    vec2 _60 = in_var_POSITION.xy * 2.0;
    float _61 = _60.x;
    float _62 = floor(_61);
    float _63 = _60.y;
    float _64 = floor(_63);
    float _66 = min(2.0, _62 + 1.0);
    float _69 = _61 - _62;
    float _71 = _64 * 3.0;
    int _73 = int(_71 + _62);
    float _80 = min(2.0, _64 + 1.0) * 3.0;
    int _82 = int(_80 + _62);
    vec4 _91 = u_EveryObject.u_eyePositions[_73] + ((u_EveryObject.u_eyePositions[int(_71 + _66)] - u_EveryObject.u_eyePositions[_73]) * _69);
    vec4 _107 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy), 0.0);
    vec4 _130 = ((_91 + (((u_EveryObject.u_eyePositions[_82] + ((u_EveryObject.u_eyePositions[int(_80 + _66)] - u_EveryObject.u_eyePositions[_82]) * _69)) - _91) * (_63 - _64))) + ((vec4(u_EveryObject.u_tileNormal.xyz * (((_107.x * 255.0) + (_107.y * 65280.0)) - 32768.0), 1.0) * u_EveryObject.u_view) - (vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_view))) * u_EveryObject.u_projection;
    float _141 = _130.w;
    float _147 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _141)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _141;
    vec4 _148 = _130;
    _148.z = _147;
    vec2 _160 = _55;
    _160.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _148;
    varying_COLOR0 = u_EveryObject.u_colour;
    varying_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    varying_TEXCOORD1 = vec2(_147, _141);
    varying_TEXCOORD2 = _160;
}

