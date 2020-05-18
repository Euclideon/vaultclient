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
    float _62 = _60.x;
    float _63 = floor(_62);
    float _64 = _60.y;
    float _65 = floor(_64);
    float _67 = fast::min(2.0, _63 + 1.0);
    float _70 = _62 - _63;
    float _71 = _64 - _65;
    float _72 = _65 * 3.0;
    int _74 = int(_72 + _63);
    int _78 = int(_72 + _67);
    float _81 = fast::min(2.0, _65 + 1.0) * 3.0;
    int _83 = int(_81 + _63);
    int _87 = int(_81 + _67);
    float4 _92 = u_EveryObject.u_eyePositions[_74] + ((u_EveryObject.u_eyePositions[_78] - u_EveryObject.u_eyePositions[_74]) * _70);
    float4 _110 = u_EveryObject.u_eyeNormals[_74] + ((u_EveryObject.u_eyeNormals[_78] - u_EveryObject.u_eyeNormals[_74]) * _70);
    float4 _126 = demTexture.sample(demSampler, (u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy)), level(0.0));
    float4 _137 = u_EveryObject.u_projection * ((_92 + (((u_EveryObject.u_eyePositions[_83] + ((u_EveryObject.u_eyePositions[_87] - u_EveryObject.u_eyePositions[_83]) * _70)) - _92) * _71)) + ((_110 + (((u_EveryObject.u_eyeNormals[_83] + ((u_EveryObject.u_eyeNormals[_87] - u_EveryObject.u_eyeNormals[_83]) * _70)) - _110) * _71)) * (((_126.x * 255.0) + (_126.y * 65280.0)) - 32768.0)));
    float _148 = _137.w;
    float _154 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _148)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _148;
    float4 _155 = _137;
    _155.z = _154;
    float2 _167 = _55;
    _167.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _155;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_154, _148);
    out.out_var_TEXCOORD2 = _167;
    return out;
}

