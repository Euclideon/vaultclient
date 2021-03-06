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
    float2 in_var_TEXCOORD1 [[user(locn2)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], texture2d<float> colourTexture [[texture(0)]], sampler colourSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _46 = colourTexture.sample(colourSampler, in.in_var_TEXCOORD0);
    float _73;
    switch (0u)
    {
        default:
        {
            if (in.in_var_TEXCOORD1.y != 0.0)
            {
                _73 = in.in_var_TEXCOORD1.x / in.in_var_TEXCOORD1.y;
                break;
            }
            _73 = log2(in.in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    out.out_var_SV_Target0 = float4(_46.xyz * in.in_var_COLOR0.xyz, _46.w * in.in_var_COLOR0.w);
    out.out_var_SV_Target1 = float4(0.0, ((step(0.0, 1.0) * 2.0) - 1.0) * _73, 0.0, 0.0);
    out.gl_FragDepth = _73;
    return out;
}

