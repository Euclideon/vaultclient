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
    float4 out_var_SV_Target [[color(0)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float3 in_var_NORMAL [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float2 in_var_TEXCOORD1 [[user(locn3)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(0)]])
{
    main0_out out = {};
    float4 _48 = texture0.sample(sampler0, in.in_var_TEXCOORD0) * in.in_var_COLOR0;
    out.out_var_SV_Target = float4(_48.xyz * ((dot(in.in_var_NORMAL, normalize(float3(0.85000002384185791015625, 0.1500000059604644775390625, 0.5))) * 0.5) + 0.5), _48.w);
    out.gl_FragDepth = log2(in.in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    return out;
}

