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
    vec4 u_uvOffsetScale;
    vec4 u_demUVOffsetScale;
    vec4 u_tileNormal;
} u_EveryObject;

uniform sampler2D SPIRV_Cross_CombineddemTexturedemSampler;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;
layout(location = 2) out vec2 out_var_TEXCOORD1;

void main()
{
    vec2 _57 = in_var_POSITION.xy * 2.0;
    float _58 = _57.x;
    float _59 = floor(_58);
    float _60 = _57.y;
    float _61 = floor(_60);
    float _63 = min(2.0, _59 + 1.0);
    float _66 = _58 - _59;
    float _68 = _61 * 3.0;
    int _70 = int(_68 + _59);
    float _77 = min(2.0, _61 + 1.0) * 3.0;
    int _79 = int(_77 + _59);
    vec4 _88 = u_EveryObject.u_eyePositions[_70] + ((u_EveryObject.u_eyePositions[int(_68 + _63)] - u_EveryObject.u_eyePositions[_70]) * _66);
    vec4 _104 = textureLod(SPIRV_Cross_CombineddemTexturedemSampler, u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in_var_POSITION.xy), 0.0);
    vec4 _127 = ((_88 + (((u_EveryObject.u_eyePositions[_79] + ((u_EveryObject.u_eyePositions[int(_77 + _63)] - u_EveryObject.u_eyePositions[_79]) * _66)) - _88) * (_60 - _61))) + ((vec4(u_EveryObject.u_tileNormal.xyz * (((_104.x * 255.0) + (_104.y * 65280.0)) - 32768.0), 1.0) * u_EveryObject.u_view) - (vec4(0.0, 0.0, 0.0, 1.0) * u_EveryObject.u_view))) * u_EveryObject.u_projection;
    float _138 = _127.w;
    float _144 = ((log2(max(9.9999999747524270787835121154785e-07, 1.0 + _138)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _138;
    vec4 _145 = _127;
    _145.z = _144;
    gl_Position = _145;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in_var_POSITION.xy);
    out_var_TEXCOORD1 = vec2(_144, _138);
}

