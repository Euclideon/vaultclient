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
    float4 u_eyeNormals[9];
    float4 u_colour;
    float4 u_objectInfo;
    float4 u_uvOffsetScale;
    float4 u_demUVOffsetScale;
};

constant float2 _56 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float2 out_var_TEXCOORD2 [[user(locn3)]];
    float2 out_var_TEXCOORD3 [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]], texture2d<float> demTexture [[texture(0)]], sampler demSampler [[sampler(0)]])
{
    main0_out out = {};
    float2 _61 = in.in_var_POSITION.xy * 2.0;
    float _63 = _61.x;
    float _64 = floor(_63);
    float _65 = _61.y;
    float _66 = floor(_65);
    float _68 = fast::min(2.0, _64 + 1.0);
    float _73 = _66 * 3.0;
    int _75 = int(_73 + _64);
    int _79 = int(_73 + _68);
    float _82 = fast::min(2.0, _66 + 1.0) * 3.0;
    int _84 = int(_82 + _64);
    int _88 = int(_82 + _68);
    float4 _91 = float4(_63 - _64);
    float4 _94 = float4(_65 - _66);
    float2 _113 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy);
    float4 _117 = demTexture.sample(demSampler, _113, level(0.0));
    float4 _128 = u_EveryObject.u_projection * (mix(mix(u_EveryObject.u_eyePositions[_75], u_EveryObject.u_eyePositions[_79], _91), mix(u_EveryObject.u_eyePositions[_84], u_EveryObject.u_eyePositions[_88], _91), _94) + (mix(mix(u_EveryObject.u_eyeNormals[_75], u_EveryObject.u_eyeNormals[_79], _91), mix(u_EveryObject.u_eyeNormals[_84], u_EveryObject.u_eyeNormals[_88], _91), _94) * (((_117.x * 255.0) + (_117.y * 65280.0)) - 32768.0)));
    float _139 = _128.w;
    float _145 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _139)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _139;
    float4 _146 = _128;
    _146.z = _145;
    float2 _158 = _56;
    _158.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _146;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_145, _139);
    out.out_var_TEXCOORD2 = _158;
    out.out_var_TEXCOORD3 = _113;
    return out;
}

