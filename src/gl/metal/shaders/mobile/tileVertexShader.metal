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
    float4 u_tileNormal;
};

constant float2 _55 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float2 out_var_TEXCOORD2 [[user(locn3)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]], texture2d<float> demTexture [[texture(0)]], sampler demSampler [[sampler(0)]])
{
    main0_out out = {};
    float2 _60 = in.in_var_POSITION.xy * 2.0;
    float _61 = _60.x;
    float _62 = floor(_61);
    float _63 = _60.y;
    float _64 = floor(_63);
    float _66 = fast::min(2.0, _62 + 1.0);
    float _69 = _61 - _62;
    float _71 = _64 * 3.0;
    int _73 = int(_71 + _62);
    float _80 = fast::min(2.0, _64 + 1.0) * 3.0;
    int _82 = int(_80 + _62);
    float4 _91 = u_EveryObject.u_eyePositions[_73] + ((u_EveryObject.u_eyePositions[int(_71 + _66)] - u_EveryObject.u_eyePositions[_73]) * _69);
    float4 _107 = demTexture.sample(demSampler, (u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy)), level(0.0));
    float4 _130 = u_EveryObject.u_projection * ((_91 + (((u_EveryObject.u_eyePositions[_82] + ((u_EveryObject.u_eyePositions[int(_80 + _66)] - u_EveryObject.u_eyePositions[_82]) * _69)) - _91) * (_63 - _64))) + ((u_EveryObject.u_view * float4(u_EveryObject.u_tileNormal.xyz * (((_107.x * 255.0) + (_107.y * 65280.0)) - 32768.0), 1.0)) - (u_EveryObject.u_view * float4(0.0, 0.0, 0.0, 1.0))));
    float _141 = _130.w;
    float _147 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _141)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _141;
    float4 _148 = _130;
    _148.z = _147;
    float2 _160 = _55;
    _160.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _148;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_147, _141);
    out.out_var_TEXCOORD2 = _160;
    return out;
}

