#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
};

struct type_u_EveryObject
{
    float4x4 u_projection;
    float4x4 u_view;
    float4 u_eyePositions[9];
    float4 u_colour;
    float4 u_objectInfo;
    float4 u_uvOffsetScale;
    float4 u_demUVOffsetScale;
    float4 u_worldNormals[9];
    float4 u_worldBitangents[9];
};

constant float2 _60 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float2 out_var_TEXCOORD2 [[user(locn3)]];
    float2 out_var_TEXCOORD3 [[user(locn4)]];
    float3 out_var_TEXCOORD4 [[user(locn5)]];
    float3 out_var_TEXCOORD5 [[user(locn6)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]], texture2d<float> demTexture [[texture(0)]], sampler demSampler [[sampler(0)]])
{
    main0_out out = {};
    float2 _65 = in.in_var_POSITION.xy * 2.0;
    float _67 = _65.x;
    float _68 = floor(_67);
    float _69 = _65.y;
    float _70 = floor(_69);
    float _72 = fast::min(2.0, _68 + 1.0);
    float _77 = _70 * 3.0;
    int _79 = int(_77 + _68);
    int _83 = int(_77 + _72);
    float _86 = fast::min(2.0, _70 + 1.0) * 3.0;
    int _88 = int(_86 + _68);
    int _92 = int(_86 + _72);
    float4 _95 = float4(_67 - _68);
    float4 _98 = float4(_69 - _70);
    float4 _100 = normalize(mix(mix(u_EveryObject.u_worldNormals[_79], u_EveryObject.u_worldNormals[_83], _95), mix(u_EveryObject.u_worldNormals[_88], u_EveryObject.u_worldNormals[_92], _95), _98));
    float2 _126 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy);
    float4 _130 = demTexture.sample(demSampler, _126, level(0.0));
    float4 _146 = u_EveryObject.u_projection * (mix(mix(u_EveryObject.u_eyePositions[_79], u_EveryObject.u_eyePositions[_83], _95), mix(u_EveryObject.u_eyePositions[_88], u_EveryObject.u_eyePositions[_92], _95), _98) + ((u_EveryObject.u_view * float4(_100.xyz, 0.0)) * ((((_130.x * 255.0) + (_130.y * 65280.0)) - 32768.0) + (in.in_var_POSITION.z * u_EveryObject.u_objectInfo.y))));
    float _157 = _146.w;
    float _163 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _157)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _157;
    float4 _164 = _146;
    _164.z = _163;
    float2 _176 = _60;
    _176.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _164;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_163, _157);
    out.out_var_TEXCOORD2 = _176;
    out.out_var_TEXCOORD3 = _126;
    out.out_var_TEXCOORD4 = _100.xyz;
    out.out_var_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_79], u_EveryObject.u_worldBitangents[_83], _95), mix(u_EveryObject.u_worldBitangents[_88], u_EveryObject.u_worldBitangents[_92], _95), _98)).xyz;
    return out;
}

