#pragma clang diagnostic ignored "-Wmissing-prototypes"

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

struct type_u_fragParams
{
    float4 u_screenParams;
    float4x4 u_inverseViewProjection;
    float4x4 u_inverseProjection;
    float4 u_outlineColour;
    float4 u_outlineParams;
    float4 u_colourizeHeightColourMin;
    float4 u_colourizeHeightColourMax;
    float4 u_colourizeHeightParams;
    float4 u_colourizeDepthColour;
    float4 u_colourizeDepthParams;
    float4 u_contourColour;
    float4 u_contourParams;
};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float4 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float2 in_var_TEXCOORD2 [[user(locn2)]];
    float2 in_var_TEXCOORD3 [[user(locn3)]];
    float2 in_var_TEXCOORD4 [[user(locn4)]];
    float2 in_var_TEXCOORD5 [[user(locn5)]];
};

// Implementation of the GLSL mod() function, which is slightly different than Metal fmod()
template<typename Tx, typename Ty>
inline Tx mod(Tx x, Ty y)
{
    return x - y * floor(x / y);
}

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
    main0_out out = {};
    float4 _77 = texture0.sample(sampler0, in.in_var_TEXCOORD1);
    float4 _81 = texture1.sample(sampler1, in.in_var_TEXCOORD1);
    float _82 = _81.x;
    float _88 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _91 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _93 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _103 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _105 = ((_88 + (_91 / (pow(2.0, _82 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear;
    float4 _110 = float4(in.in_var_TEXCOORD0.xy, _105, 1.0);
    float4 _111 = u_fragParams.u_inverseViewProjection * _110;
    float4 _114 = _111 / float4(_111.w);
    float4 _117 = u_fragParams.u_inverseProjection * _110;
    float3 _121 = _77.xyz;
    float _124 = _114.z;
    float3 _200 = mix(mix(mix(mix(mix(_121, u_fragParams.u_colourizeHeightColourMin.xyz, float3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_121, u_fragParams.u_colourizeHeightColourMax.xyz, float3(u_fragParams.u_colourizeHeightColourMax.w)), float3(fast::clamp((_124 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, float3(fast::clamp((length((_117 / float4(_117.w)).xyz) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, fast::clamp(abs((fract(float3(_124 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + float3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - float3(3.0)) - float3(1.0), float3(0.0), float3(1.0)) * 1.0, float3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, float3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_124), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _350;
    float4 _351;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        float4 _222 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD1.x * 2.0) - 1.0, (in.in_var_TEXCOORD1.y * 2.0) - 1.0, _105, 1.0);
        float4 _227 = texture1.sample(sampler1, in.in_var_TEXCOORD2);
        float _228 = _227.x;
        float4 _230 = texture1.sample(sampler1, in.in_var_TEXCOORD3);
        float _231 = _230.x;
        float4 _233 = texture1.sample(sampler1, in.in_var_TEXCOORD4);
        float _234 = _233.x;
        float4 _236 = texture1.sample(sampler1, in.in_var_TEXCOORD5);
        float _237 = _236.x;
        float4 _252 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD2.x * 2.0) - 1.0, (in.in_var_TEXCOORD2.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _228 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _267 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD3.x * 2.0) - 1.0, (in.in_var_TEXCOORD3.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _231 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _282 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD4.x * 2.0) - 1.0, (in.in_var_TEXCOORD4.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _234 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _297 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD5.x * 2.0) - 1.0, (in.in_var_TEXCOORD5.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _237 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float3 _310 = (_222 / float4(_222.w)).xyz;
        float4 _347 = mix(float4(_200, _82), float4(mix(_200.xyz, u_fragParams.u_outlineColour.xyz, float3(u_fragParams.u_outlineColour.w)), fast::min(fast::min(fast::min(_228, _231), _234), _237)), float4(1.0 - (((step(length(_310 - (_252 / float4(_252.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_310 - (_267 / float4(_267.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_310 - (_282 / float4(_282.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_310 - (_297 / float4(_297.w)).xyz), u_fragParams.u_outlineParams.y))));
        _350 = _347.w;
        _351 = float4(_347.x, _347.y, _347.z, _77.w);
    }
    else
    {
        _350 = _82;
        _351 = float4(_200.x, _200.y, _200.z, _77.w);
    }
    out.out_var_SV_Target = float4(_351.xyz, 1.0);
    out.gl_FragDepth = _350;
    return out;
}

