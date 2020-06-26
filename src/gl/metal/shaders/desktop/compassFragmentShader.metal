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
    float3 in_var_COLOR0 [[user(locn0)]];
    float4 in_var_COLOR1 [[user(locn1)]];
    float3 in_var_COLOR2 [[user(locn2)]];
    float4 in_var_COLOR3 [[user(locn3)]];
    float2 in_var_TEXCOORD0 [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]])
{
    main0_out out = {};
    float3 _51 = (in.in_var_COLOR0 * float3(2.0)) - float3(1.0);
    float _77 = log2(in.in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out.out_var_SV_Target0 = float4(((in.in_var_COLOR1.xyz * (0.5 + (dot(in.in_var_COLOR2, _51) * (-0.5)))) + (float3(1.0, 1.0, 0.89999997615814208984375) * pow(fast::max(0.0, -dot(-normalize(in.in_var_COLOR3.xyz / float3(in.in_var_COLOR3.w)), _51)), 60.0))) * in.in_var_COLOR1.w, 1.0);
    out.out_var_SV_Target1 = float4(0.0, ((step(0.0, in.in_var_COLOR0.z) * 2.0) - 1.0) * _77, in.in_var_COLOR0.xy);
    out.gl_FragDepth = _77;
    return out;
}

