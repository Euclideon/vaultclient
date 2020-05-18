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

vec2 _55;

void main()
{
    vec2 _60 = in_var_POSITION.xy * 2.0;
    float _62 = _60.x;
    float _63 = floor(_62);
    float _64 = _60.y;
    float _65 = floor(_64);
    float _67 = min(2.0, _63 + 1.0);
    float _70 = _62 - _63;
    float _71 = _64 - _65;
    float _72 = _65 * 3.0;
    int _74 = int(_72 + _63);
    int _78 = int(_72 + _67);
    float _81 = min(2.0, _65 + 1.0) * 3.0;
    int _83 = int(_81 + _63);
    int _87 = int(_81 + _67);
    vec4 _92 = u_EveryObject.u_eyePositions[_74] + ((u_EveryObject.u_eyePositions[_78] - u_EveryObject.u_eyePositions[_74]) * _70);
    vec4 _110 = u_EveryObject.u_eyeNormals[_74] + ((u_EveryObject.u_eyeNormals[_78] - u_EveryObject.u_eyeNormals[_74]) * _70);
    vec4 _126 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy), 0.0);
    vec4 _137 = ((_92 + (((u_EveryObject.u_eyePositions[_83] + ((u_EveryObject.u_eyePositions[_87] - u_EveryObject.u_eyePositions[_83]) * _70)) - _92) * _71)) + ((_110 + (((u_EveryObject.u_eyeNormals[_83] + ((u_EveryObject.u_eyeNormals[_87] - u_EveryObject.u_eyeNormals[_83]) * _70)) - _110) * _71)) * (((_126.x * 255.0) + (_126.y * 65280.0)) - 32768.0))) * u_EveryObject.u_projection;
    float _148 = _137.w;
    float _154 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _148)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _148;
    vec4 _155 = _137;
    _155.z = _154;
    vec2 _167 = _55;
    _167.x = u_EveryObject.u_objectInfo.x;
    gl_Position = _155;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = vec2(_154, _148);
    out_var_TEXCOORD2 = _167;
}

