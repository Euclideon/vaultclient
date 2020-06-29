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

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float4 in_var_COLOR0 [[user(locn0)]];
    float2 in_var_TEXCOORD0 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]])
{
    main0_out out = {};
    float _37 = log2(in.in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out.out_var_SV_Target0 = in.in_var_COLOR0;
    out.out_var_SV_Target1 = float4(0.0, ((step(0.0, 0.0) * 2.0) - 1.0) * _37, 0.0, 0.0);
    out.gl_FragDepth = _37;
    return out;
}

