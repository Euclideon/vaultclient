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
    float4 u_eyePositions[4];
    float4 u_colour;
    float4 u_objectInfo;
    float4 u_uvOffsetScale;
    float4 u_demUVOffsetScale;
    float4 u_worldNormals[4];
    float4 u_worldBitangents[4];
};

constant float2 _59 = {};

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
    float2 _64 = in.in_var_POSITION.xy * 1.0;
    float _66 = _64.x;
    float _67 = floor(_66);
    float _68 = _64.y;
    float _69 = floor(_68);
    float _71 = fast::min(1.0, _67 + 1.0);
    float _76 = _69 * 2.0;
    int _78 = int(_76 + _67);
    int _82 = int(_76 + _71);
    float _85 = fast::min(1.0, _69 + 1.0) * 2.0;
    int _87 = int(_85 + _67);
    int _91 = int(_85 + _71);
    float4 _94 = float4(_66 - _67);
    float4 _97 = float4(_68 - _69);
    float4 _99 = normalize(mix(mix(u_EveryObject.u_worldNormals[_78], u_EveryObject.u_worldNormals[_82], _94), mix(u_EveryObject.u_worldNormals[_87], u_EveryObject.u_worldNormals[_91], _94), _97));
    float2 _125 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy);
    float4 _129 = demTexture.sample(demSampler, _125, level(0.0));
    float4 _145 = u_EveryObject.u_projection * (mix(mix(u_EveryObject.u_eyePositions[_78], u_EveryObject.u_eyePositions[_82], _94), mix(u_EveryObject.u_eyePositions[_87], u_EveryObject.u_eyePositions[_91], _94), _97) + ((u_EveryObject.u_view * float4(_99.xyz, 0.0)) * ((((_129.x * 255.0) + (_129.y * 65280.0)) - 32768.0) + (in.in_var_POSITION.z * u_EveryObject.u_objectInfo.y))));
    float _156 = _145.w;
    float _162 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _156)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _156;
    float4 _163 = _145;
    _163.z = _162;
    float2 _175 = _59;
    _175.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _163;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_162, _156);
    out.out_var_TEXCOORD2 = _175;
    out.out_var_TEXCOORD3 = _125;
    out.out_var_TEXCOORD4 = _99.xyz;
    out.out_var_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_78], u_EveryObject.u_worldBitangents[_82], _94), mix(u_EveryObject.u_worldBitangents[_87], u_EveryObject.u_worldBitangents[_91], _94), _97)).xyz;
    return out;
}

