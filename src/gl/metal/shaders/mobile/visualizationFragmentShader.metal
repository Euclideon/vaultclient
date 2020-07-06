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
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
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

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> sceneColourTexture [[texture(0)]], texture2d<float> sceneNormalTexture [[texture(1)]], texture2d<float> sceneDepthTexture [[texture(2)]], sampler sceneColourSampler [[sampler(0)]], sampler sceneNormalSampler [[sampler(1)]], sampler sceneDepthSampler [[sampler(2)]])
{
    main0_out out = {};
    float4 _83 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD1);
    float4 _87 = sceneNormalTexture.sample(sceneNormalSampler, in.in_var_TEXCOORD1);
    float4 _91 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1);
    float _92 = _91.x;
    float4 _121 = u_fragParams.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane)) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _92 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0);
    float3 _125 = _83.xyz;
    float3 _126 = (_121 / float4(_121.w)).xyz;
    float _133 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    float3 _135 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_126, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _133);
    float _143 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _135) * ((float(dot(_135, _135) < _133) * 2.0) - 1.0);
    float _193 = abs(_143);
    float3 _219 = mix(mix(mix(mix(mix(_125, u_fragParams.u_colourizeHeightColourMin.xyz, float3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_125, u_fragParams.u_colourizeHeightColourMax.xyz, float3(u_fragParams.u_colourizeHeightColourMax.w)), float3(fast::clamp((_143 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, float3(fast::clamp((length(_126) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, fast::clamp(abs((fract(float3(_193 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + float3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - float3(3.0)) - float3(1.0), float3(0.0), float3(1.0)) * 1.0, float3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, float3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_193), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _284;
    float4 _285;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        float4 _235 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD2);
        float _236 = _235.x;
        float4 _238 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD3);
        float _239 = _238.x;
        float4 _241 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD4);
        float _242 = _241.x;
        float4 _244 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD5);
        float _245 = _244.x;
        float _248 = 0.0005000000237487256526947021484375 + ((u_fragParams.u_outlineParams.y * _92) * 0.001000000047497451305389404296875);
        float4 _281 = mix(float4(_219, _92), float4(mix(_219.xyz, u_fragParams.u_outlineColour.xyz, float3(u_fragParams.u_outlineColour.w)), fast::min(fast::min(fast::min(_236, _239), _242), _245)), float4(1.0 - (((step(abs(_236 - _92), _248) * step(abs(_239 - _92), _248)) * step(abs(_242 - _92), _248)) * step(abs(_245 - _92), _248))));
        _284 = _281.w;
        _285 = float4(_281.x, _281.y, _281.z, _83.w);
    }
    else
    {
        _284 = _92;
        _285 = float4(_219.x, _219.y, _219.z, _83.w);
    }
    out.out_var_SV_Target0 = float4(_285.xyz, 1.0);
    out.out_var_SV_Target1 = _87;
    out.gl_FragDepth = _284;
    return out;
}

