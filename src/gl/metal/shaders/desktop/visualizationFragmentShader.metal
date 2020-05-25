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
    float4 _90 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD1);
    float4 _94 = sceneNormalTexture.sample(sceneNormalSampler, in.in_var_TEXCOORD1);
    float4 _98 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1);
    float _99 = _98.x;
    float _105 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _108 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _110 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _120 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _122 = ((_105 + (_108 / (pow(2.0, _99 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear;
    float4 _129 = u_fragParams.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy, _122, 1.0);
    float4 _132 = _129 / float4(_129.w);
    float4 _140 = u_fragParams.u_inverseProjection * float4(in.in_var_TEXCOORD0.xy + float2(0.0, u_fragParams.u_screenParams.y), _122, 1.0);
    float3 _144 = _90.xyz;
    float3 _145 = _132.xyz;
    float _152 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    float3 _154 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_145, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _152);
    float _162 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _154) * ((float(dot(_154, _154) < _152) * 2.0) - 1.0);
    float _205 = length((_140 / float4(_140.w)) - _132);
    float3 _269;
    switch (0u)
    {
        default:
        {
            if (_205 == 0.0)
            {
                _269 = float3(1.0, 1.0, 0.0);
                break;
            }
            float3 _222;
            _222 = float3(0.0);
            for (int _225 = 0; _225 < 16; )
            {
                _222 += (fast::clamp(abs((fract(float3(((abs(_162) + (_205 * (-4.0))) + ((_205 * 0.533333361148834228515625) * float(_225))) * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + float3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - float3(3.0)) - float3(1.0), float3(0.0), float3(1.0)) * 1.0);
                _225++;
                continue;
            }
            float _247 = _205 * 16.0;
            _269 = mix(mix(mix(mix(mix(_144, u_fragParams.u_colourizeHeightColourMin.xyz, float3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_144, u_fragParams.u_colourizeHeightColourMax.xyz, float3(u_fragParams.u_colourizeHeightColourMax.w)), float3(fast::clamp((_162 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, float3(fast::clamp((length(_145) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, _222 * float3(0.0625), float3(fast::clamp(u_fragParams.u_contourParams.w * fast::clamp(u_fragParams.u_contourParams.z / _247, 0.0, 1.0), 0.0, 1.0))), u_fragParams.u_contourColour.xyz, float3(((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(abs(_162)), u_fragParams.u_contourParams.x))) * fast::clamp(u_fragParams.u_contourParams.x / _247, 0.0, 1.0)) * u_fragParams.u_contourColour.w));
            break;
        }
    }
    float _419;
    float4 _420;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        float4 _291 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD1.x * 2.0) - 1.0, (in.in_var_TEXCOORD1.y * 2.0) - 1.0, _122, 1.0);
        float4 _296 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD2);
        float _297 = _296.x;
        float4 _299 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD3);
        float _300 = _299.x;
        float4 _302 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD4);
        float _303 = _302.x;
        float4 _305 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD5);
        float _306 = _305.x;
        float4 _321 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD2.x * 2.0) - 1.0, (in.in_var_TEXCOORD2.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _297 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _336 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD3.x * 2.0) - 1.0, (in.in_var_TEXCOORD3.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _300 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _351 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD4.x * 2.0) - 1.0, (in.in_var_TEXCOORD4.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _303 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float4 _366 = u_fragParams.u_inverseProjection * float4((in.in_var_TEXCOORD5.x * 2.0) - 1.0, (in.in_var_TEXCOORD5.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _306 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0);
        float3 _379 = (_291 / float4(_291.w)).xyz;
        float4 _416 = mix(float4(_269, _99), float4(mix(_269.xyz, u_fragParams.u_outlineColour.xyz, float3(u_fragParams.u_outlineColour.w)), fast::min(fast::min(fast::min(_297, _300), _303), _306)), float4(1.0 - (((step(length(_379 - (_321 / float4(_321.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_379 - (_336 / float4(_336.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_351 / float4(_351.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_366 / float4(_366.w)).xyz), u_fragParams.u_outlineParams.y))));
        _419 = _416.w;
        _420 = float4(_416.x, _416.y, _416.z, _90.w);
    }
    else
    {
        _419 = _99;
        _420 = float4(_269.x, _269.y, _269.z, _90.w);
    }
    out.out_var_SV_Target0 = float4(_420.xyz, 1.0);
    out.out_var_SV_Target1 = _94;
    out.gl_FragDepth = _419;
    return out;
}

