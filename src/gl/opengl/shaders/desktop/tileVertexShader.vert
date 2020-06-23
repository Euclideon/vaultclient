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
    vec4 u_eyeNormals[9];
    vec4 u_colour;
    vec4 u_objectInfo;
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
} u_EveryObject;

uniform sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;
layout(location = 3) out vec2 out_var_TEXCOORD2;
layout(location = 4) out vec2 out_var_TEXCOORD3;

vec2 _57;

void main()
{
    vec2 _62 = in_var_POSITION.xy * 2.0;
    float _64 = _62.x;
    float _65 = floor(_64);
    float _66 = _62.y;
    float _67 = floor(_66);
    float _69 = min(2.0, _65 + 1.0);
    float _74 = _67 * 3.0;
    int _76 = int(_74 + _65);
    int _80 = int(_74 + _69);
    float _83 = min(2.0, _67 + 1.0) * 3.0;
    int _85 = int(_83 + _65);
    int _89 = int(_83 + _69);
    vec4 _92 = vec4(_64 - _65);
    vec4 _95 = vec4(_66 - _67);
    vec2 _114 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy);
    vec4 _118 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, _114, 0.0);
    vec4 _135 = (mix(mix(u_EveryObject.u_eyePositions[_76], u_EveryObject.u_eyePositions[_80], _92), mix(u_EveryObject.u_eyePositions[_85], u_EveryObject.u_eyePositions[_89], _92), _95) + (mix(mix(u_EveryObject.u_eyeNormals[_76], u_EveryObject.u_eyeNormals[_80], _92), mix(u_EveryObject.u_eyeNormals[_85], u_EveryObject.u_eyeNormals[_89], _92), _95) * ((((_118.x * 255.0) + (_118.y * 65280.0)) - 32768.0) + ((in_var_POSITION.z * u_EveryObject.u_objectInfo.y) * 3000000.0)))) * u_EveryObject.u_projection;
    float _146 = _135.w;
    float _152 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _146)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _146;
    vec4 _153 = _135;
    _153.z = _152;
    vec2 _165 = _57;
    _165.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _153;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = vec2(_152, _146);
    out_var_TEXCOORD2 = _165;
    out_var_TEXCOORD3 = _114;
}

