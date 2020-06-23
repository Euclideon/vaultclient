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

constant float2 _57 = {};

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
    float2 _62 = in.in_var_POSITION.xy * 2.0;
    float _64 = _62.x;
    float _65 = floor(_64);
    float _66 = _62.y;
    float _67 = floor(_66);
    float _69 = fast::min(2.0, _65 + 1.0);
    float _74 = _67 * 3.0;
    int _76 = int(_74 + _65);
    int _80 = int(_74 + _69);
    float _83 = fast::min(2.0, _67 + 1.0) * 3.0;
    int _85 = int(_83 + _65);
    int _89 = int(_83 + _69);
    float4 _92 = float4(_64 - _65);
    float4 _95 = float4(_66 - _67);
    float2 _114 = u_EveryObject.u_demUVOffsetScale.xy + (u_EveryObject.u_demUVOffsetScale.zw * in.in_var_POSITION.xy);
    float4 _118 = demTexture.sample(demSampler, _114, level(0.0));
    float4 _135 = u_EveryObject.u_projection * (mix(mix(u_EveryObject.u_eyePositions[_76], u_EveryObject.u_eyePositions[_80], _92), mix(u_EveryObject.u_eyePositions[_85], u_EveryObject.u_eyePositions[_89], _92), _95) + (mix(mix(u_EveryObject.u_eyeNormals[_76], u_EveryObject.u_eyeNormals[_80], _92), mix(u_EveryObject.u_eyeNormals[_85], u_EveryObject.u_eyeNormals[_89], _92), _95) * ((((_118.x * 255.0) + (_118.y * 65280.0)) - 32768.0) + ((in.in_var_POSITION.z * u_EveryObject.u_objectInfo.y) * 3000000.0))));
    float _146 = _135.w;
    float _152 = ((log2(fast::max(9.9999999747524270787835121154785e-07, 1.0 + _146)) * ((u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear) / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0))) + u_cameraPlaneParams.u_clipZNear) * _146;
    float4 _153 = _135;
    _153.z = _152;
    float2 _165 = _57;
    _165.x = u_EveryObject.u_objectInfo.x;
    out.gl_Position = _153;
    out.out_var_COLOR0 = u_EveryObject.u_colour;
    out.out_var_TEXCOORD0 = u_EveryObject.u_uvOffsetScale.xy + (u_EveryObject.u_uvOffsetScale.zw * in.in_var_POSITION.xy);
    out.out_var_TEXCOORD1 = float2(_152, _146);
    out.out_var_TEXCOORD2 = _165;
    out.out_var_TEXCOORD3 = _114;
    return out;
}

