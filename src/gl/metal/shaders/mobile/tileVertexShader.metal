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

constant float2 _57 = {};

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
    float2 _61 = in.in_var_POSITION.xy * 1.0;
    float _63 = _61.x;
    float _64 = floor(_63);
    float _65 = _61.y;
    float _66 = floor(_65);
    float _68 = fast::min(1.0, _64 + 1.0);
    float _73 = _66 * 2.0;
    int _75 = int(_73 + _64);
    int _79 = int(_73 + _68);
    float _82 = fast::min(1.0, _66 + 1.0) * 2.0;
    int _84 = int(_82 + _64);
    int _88 = int(_82 + _68);
    float4 _91 = float4(_63 - _64);
    float4 _94 = float4(_65 - _66);
    float4 _96 = normalize(mix(mix(u_EveryObject.u_worldNormals[_75], u_EveryObject.u_worldNormals[_79], _91), mix(u_EveryObject.u_worldNormals[_84], u_EveryObject.u_worldNormals[_88], _91), _94));
    float3 _97 = _96.xyz;
    float4 _109 = mix(mix(u_EveryObject.u_eyePositions[_75], u_EveryObject.u_eyePositions[_79], _91), mix(u_EveryObject.u_eyePositions[_84], u_EveryObject.u_eyePositions[_88], _91), _94);
    float2 _122 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy);
    float4 _154 = u_EveryObject.u_projection * (float4(mix(_109.xyz, (u_EveryObject.u_view * float4(_97 * _109.w, 1.0)).xyz, float3(u_EveryObject.u_objectInfo.z)), 1.0) + ((u_EveryObject.u_view * float4(_96.xyz, 0.0)) * (demTexture.sample(demSampler, _122, level(0.0)).x + (in.in_var_POSITION.z * u_EveryObject.u_objectInfo.y))));
    float _165 = _154.w;
    float _171 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _165)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _165;
    float4 _172 = _154;
    _172.z = _171;
    float2 _184 = _57;
    _184.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _172;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_171, _165);
    out.out_var_TEXCOORD2 = _184;
    out.out_var_TEXCOORD3 = _122;
    out.out_var_TEXCOORD4 = _97;
    out.out_var_TEXCOORD5 = normalize(mix(mix(u_EveryObject.u_worldBitangents[_75], u_EveryObject.u_worldBitangents[_79], _91), mix(u_EveryObject.u_worldBitangents[_84], u_EveryObject.u_worldBitangents[_88], _91), _94)).xyz;
    return out;
}

