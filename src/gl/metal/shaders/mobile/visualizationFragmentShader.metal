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
    float4 u_eyeToEarthSurfaceEyeSpace;
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

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> sceneColourTexture [[texture(0)]], texture2d<float> sceneDepthTexture [[texture(1)]], sampler sceneColourSampler [[sampler(0)]], sampler sceneDepthSampler [[sampler(1)]])
{
    main0_out out = {};
    float4 _78 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD1);
    float4 _82 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1);
    float _83 = _82.x;
    float _89 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _92 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _94 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _104 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _106 = ((_89 + (_92 / (pow(2.0, _83 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear;
    float4 _112 = u_fragParams.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy, _106, 1.0);
    float3 _116 = _78.xyz;
    float3 _117 = (_112 / float4(_112.w)).xyz;
    float _124 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    float3 _126 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_117, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _124);
    float _132 = mix(length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _126), 0.0, float(dot(_126, _126) > _124));
    float3 _207 = mix(mix(mix(mix(mix(_116, u_fragParams.u_colourizeHeightColourMin.xyz, float3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_116, u_fragParams.u_colourizeHeightColourMax.xyz, float3(u_fragParams.u_colourizeHeightColourMax.w)), float3(fast::clamp((_132 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, float3(fast::clamp((length(_117) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, fast::clamp(abs((fract(float3(_132 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + float3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - float3(3.0)) - float3(1.0), float3(0.0), float3(1.0)) * 1.0, float3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, float3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_132), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _357;
    float4 _358;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        float4 _229 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD1.x * 2.0) - 1.0, (in.in_var_TEXCOORD1.y * 2.0) - 1.0, _106, 1.0);
        float4 _234 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD2);
        float _235 = _234.x;
        float4 _237 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD3);
        float _238 = _237.x;
        float4 _240 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD4);
        float _241 = _240.x;
        float4 _243 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD5);
        float _244 = _243.x;
        float4 _259 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD2.x * 2.0) - 1.0, (in.in_var_TEXCOORD2.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _235 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _274 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD3.x * 2.0) - 1.0, (in.in_var_TEXCOORD3.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _238 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _289 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD4.x * 2.0) - 1.0, (in.in_var_TEXCOORD4.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _241 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _304 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD5.x * 2.0) - 1.0, (in.in_var_TEXCOORD5.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _244 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float3 _317 = (_229 / float4(_229.w)).xyz;
        float4 _354 = mix(float4(_207, _83), float4(mix(_207.xyz, u_fragParams.u_outlineColour.xyz, float3(u_fragParams.u_outlineColour.w)), fast::min(fast::min(fast::min(_235, _238), _241), _244)), float4(1.0 - (((step(length(_317 - (_259 / float4(_259.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_317 - (_274 / float4(_274.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_289 / float4(_289.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_304 / float4(_304.w)).xyz), u_fragParams.u_outlineParams.y))));
        _357 = _354.w;
        _358 = float4(_354.x, _354.y, _354.z, _78.w);
    }
    else
    {
        _357 = _83;
        _358 = float4(_207.x, _207.y, _207.z, _78.w);
    }
    float4 _363 = float4(_358.xyz, 1.0);
    _363.w = _357;
    out.out_var_SV_Target = _363;
    out.gl_FragDepth = _357;
    return out;
}

