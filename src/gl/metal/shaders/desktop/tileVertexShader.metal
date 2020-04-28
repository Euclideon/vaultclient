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
    float4 u_uvOffsetScale;
    float4 u_demUVOffsetScale;
    float4 u_tileNormal;
};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float2 out_var_TEXCOORD1 [[user(locn2)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_EveryObject& u_EveryObject [[buffer(1)]], texture2d<float> demTexture [[texture(0)]], sampler demSampler [[sampler(0)]])
{
    main0_out out = {};
    float2 _57 = in.in_var_POSITION.xy * 2.0;
    float _58 = _57.x;
    float _59 = floor(_58);
    float _60 = _57.y;
    float _61 = floor(_60);
    float _63 = fast::min(2.0, _59 + 1.0);
    float _66 = _58 - _59;
    float _68 = _61 * 3.0;
    int _70 = int(_68 + _59);
    float _77 = fast::min(2.0, _61 + 1.0) * 3.0;
    int _79 = int(_77 + _59);
    float4 _88 = u_EveryObject.u_eyePositions[_70] + ((u_EveryObject.u_eyePositions[int(_68 + _63)] - u_EveryObject.u_eyePositions[_70]) * _66);
    float4 _104 = demTexture.sample(demSampler, (u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy)), level(0.0));
    float4 _127 = u_EveryObject.u_projection * ((_88 + (((u_EveryObject.u_eyePositions[_79] + ((u_EveryObject.u_eyePositions[int(_77 + _63)] - u_EveryObject.u_eyePositions[_79]) * _66)) - _88) * (_60 - _61))) + ((u_EveryObject.u_view * float4(u_EveryObject.u_tileNormal.xyz * (((_104.x * 255.0) + (_104.y * 65280.0)) - 32768.0), 1.0)) - (u_EveryObject.u_view * float4(0.0, 0.0, 0.0, 1.0))));
    float _138 = _127.w;
    float _144 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _138)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _138;
    float4 _145 = _127;
    _145.z = _144;
    out.gl_Position = _145;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_144, _138);
    return out;
}

