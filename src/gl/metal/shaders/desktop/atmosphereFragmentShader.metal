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
    float4 u_camera;
    float4 u_whitePoint;
    float4 u_earthCenter;
    float4 u_sunDirection;
    float4 u_sunSize;
    float4x4 u_inverseViewProjection;
    float4x4 u_inverseProjection;
};

constant float4 _108 = {};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float3 in_var_TEXCOORD1 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> transmittanceTexture [[texture(0)]], texture3d<float> scatteringTexture [[texture(1)]], texture2d<float> irradianceTexture [[texture(2)]], texture2d<float> sceneColourTexture [[texture(3)]], texture2d<float> sceneNormalTexture [[texture(4)]], texture2d<float> sceneDepthTexture [[texture(5)]], sampler transmittanceSampler [[sampler(0)]], sampler scatteringSampler [[sampler(1)]], sampler irradianceSampler [[sampler(2)]], sampler sceneColourSampler [[sampler(3)]], sampler sceneNormalSampler [[sampler(4)]], sampler sceneDepthSampler [[sampler(5)]])
{
    main0_out out = {};
    float _116 = u_fragParams.u_earthCenter.w + 60000.0;
    float3 _129 = normalize(in.in_var_TEXCOORD1);
    float4 _133 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0);
    float4 _137 = sceneNormalTexture.sample(sceneNormalSampler, in.in_var_TEXCOORD0);
    float4 _141 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _142 = _141.x;
    float _147 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float3 _161 = pow(abs(_133.xyz), float3(2.2000000476837158203125));
    float _167 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _147) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _142 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _147))) * u_cameraPlaneParams.s_CameraFarPlane;
    bool _170 = _142 < 0.64999997615814208984375;
    float3 _761;
    if (_170)
    {
        float3 _173 = (u_fragParams.u_camera.xyz + (_129 * _167)) - u_fragParams.u_earthCenter.xyz;
        float3 _174 = normalize(_173);
        float _177 = length(_173);
        float _179 = dot(_173, u_fragParams.u_sunDirection.xyz) / _177;
        float _181 = _116 - u_fragParams.u_earthCenter.w;
        float4 _192 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_179 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_177 - u_fragParams.u_earthCenter.w) / _181) * 0.9375)));
        float _199 = u_fragParams.u_earthCenter.w / _177;
        float _205 = _116 * _116;
        float _206 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        float _208 = sqrt(_205 - _206);
        float _209 = _177 * _177;
        float _212 = sqrt(fast::max(_209 - _206, 0.0));
        float _223 = _116 - _177;
        float4 _236 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_177) * _179) + sqrt(fast::max((_209 * ((_179 * _179) - 1.0)) + _205, 0.0)), 0.0) - _223) / ((_212 + _208) - _223)) * 0.99609375), 0.0078125 + ((_212 / _208) * 0.984375)));
        float _255 = mix(_167 * 0.64999997615814208984375, 0.0, pow(_142 * 1.53846156597137451171875, 8.0));
        float3 _256 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
        float3 _259 = normalize(_173 - _256);
        float _260 = length(_256);
        float _261 = dot(_256, _259);
        float _268 = (-_261) - sqrt(((_261 * _261) - (_260 * _260)) + _205);
        bool _269 = _268 > 0.0;
        float3 _275;
        float _276;
        if (_269)
        {
            _275 = _256 + (_259 * _268);
            _276 = _261 + _268;
        }
        else
        {
            _275 = _256;
            _276 = _261;
        }
        float _295;
        float _277 = _269 ? _116 : _260;
        float _278 = _276 / _277;
        float _279 = dot(_275, u_fragParams.u_sunDirection.xyz);
        float _280 = _279 / _277;
        float _281 = dot(_259, u_fragParams.u_sunDirection.xyz);
        float _283 = length(_173 - _275);
        float _285 = _277 * _277;
        float _288 = _285 * ((_278 * _278) - 1.0);
        bool _291 = (_278 < 0.0) && ((_288 + _206) >= 0.0);
        float3 _420;
        switch (0u)
        {
            default:
            {
                _295 = (2.0 * _277) * _278;
                float _300 = fast::clamp(sqrt((_283 * (_283 + _295)) + _285), u_fragParams.u_earthCenter.w, _116);
                float _303 = fast::clamp((_276 + _283) / _300, -1.0, 1.0);
                if (_291)
                {
                    float _361 = -_303;
                    float _362 = _300 * _300;
                    float _365 = sqrt(fast::max(_362 - _206, 0.0));
                    float _376 = _116 - _300;
                    float _390 = -_278;
                    float _393 = sqrt(fast::max(_285 - _206, 0.0));
                    float _404 = _116 - _277;
                    _420 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_300) * _361) + sqrt(fast::max((_362 * ((_361 * _361) - 1.0)) + _205, 0.0)), 0.0) - _376) / ((_365 + _208) - _376)) * 0.99609375), 0.0078125 + ((_365 / _208) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_277) * _390) + sqrt(fast::max((_285 * ((_390 * _390) - 1.0)) + _205, 0.0)), 0.0) - _404) / ((_393 + _208) - _404)) * 0.99609375), 0.0078125 + ((_393 / _208) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _309 = sqrt(fast::max(_285 - _206, 0.0));
                    float _317 = _116 - _277;
                    float _331 = _300 * _300;
                    float _334 = sqrt(fast::max(_331 - _206, 0.0));
                    float _345 = _116 - _300;
                    _420 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_277) * _278) + sqrt(fast::max(_288 + _205, 0.0)), 0.0) - _317) / ((_309 + _208) - _317)) * 0.99609375), 0.0078125 + ((_309 / _208) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_300) * _303) + sqrt(fast::max((_331 * ((_303 * _303) - 1.0)) + _205, 0.0)), 0.0) - _345) / ((_334 + _208) - _345)) * 0.99609375), 0.0078125 + ((_334 / _208) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _423 = sqrt(fast::max(_285 - _206, 0.0));
        float _424 = _423 / _208;
        float _426 = 0.015625 + (_424 * 0.96875);
        float _429 = ((_276 * _276) - _285) + _206;
        float _462;
        if (_291)
        {
            float _452 = _277 - u_fragParams.u_earthCenter.w;
            _462 = 0.5 - (0.5 * (0.0078125 + (((_423 == _452) ? 0.0 : ((((-_276) - sqrt(fast::max(_429, 0.0))) - _452) / (_423 - _452))) * 0.984375)));
        }
        else
        {
            float _439 = _116 - _277;
            _462 = 0.5 + (0.5 * (0.0078125 + (((((-_276) + sqrt(fast::max(_429 + (_208 * _208), 0.0))) - _439) / ((_423 + _208) - _439)) * 0.984375)));
        }
        float _467 = -u_fragParams.u_earthCenter.w;
        float _474 = _208 - _181;
        float _475 = (fast::max((_467 * _280) + sqrt(fast::max((_206 * ((_280 * _280) - 1.0)) + _205, 0.0)), 0.0) - _181) / _474;
        float _477 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _474;
        float _484 = 0.015625 + ((fast::max(1.0 - (_475 / _477), 0.0) / (1.0 + _475)) * 0.96875);
        float _486 = (_281 + 1.0) * 3.5;
        float _487 = floor(_486);
        float _488 = _486 - _487;
        float _492 = _487 + 1.0;
        float4 _498 = scatteringTexture.sample(scatteringSampler, float3((_487 + _484) * 0.125, _462, _426));
        float _499 = 1.0 - _488;
        float4 _502 = scatteringTexture.sample(scatteringSampler, float3((_492 + _484) * 0.125, _462, _426));
        float4 _504 = (_498 * _499) + (_502 * _488);
        float3 _505 = _504.xyz;
        float3 _518;
        switch (0u)
        {
            default:
            {
                float _508 = _504.x;
                if (_508 == 0.0)
                {
                    _518 = float3(0.0);
                    break;
                }
                _518 = (((_505 * _504.w) / float3(_508)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _520 = fast::max(_283 - _255, 0.0);
        float _525 = fast::clamp(sqrt((_520 * (_520 + _295)) + _285), u_fragParams.u_earthCenter.w, _116);
        float _526 = _276 + _520;
        float _529 = (_279 + (_520 * _281)) / _525;
        float _530 = _525 * _525;
        float _533 = sqrt(fast::max(_530 - _206, 0.0));
        float _534 = _533 / _208;
        float _536 = 0.015625 + (_534 * 0.96875);
        float _539 = ((_526 * _526) - _530) + _206;
        float _572;
        if (_291)
        {
            float _562 = _525 - u_fragParams.u_earthCenter.w;
            _572 = 0.5 - (0.5 * (0.0078125 + (((_533 == _562) ? 0.0 : ((((-_526) - sqrt(fast::max(_539, 0.0))) - _562) / (_533 - _562))) * 0.984375)));
        }
        else
        {
            float _549 = _116 - _525;
            _572 = 0.5 + (0.5 * (0.0078125 + (((((-_526) + sqrt(fast::max(_539 + (_208 * _208), 0.0))) - _549) / ((_533 + _208) - _549)) * 0.984375)));
        }
        float _583 = (fast::max((_467 * _529) + sqrt(fast::max((_206 * ((_529 * _529) - 1.0)) + _205, 0.0)), 0.0) - _181) / _474;
        float _590 = 0.015625 + ((fast::max(1.0 - (_583 / _477), 0.0) / (1.0 + _583)) * 0.96875);
        float4 _598 = scatteringTexture.sample(scatteringSampler, float3((_487 + _590) * 0.125, _572, _536));
        float4 _601 = scatteringTexture.sample(scatteringSampler, float3((_492 + _590) * 0.125, _572, _536));
        float4 _603 = (_598 * _499) + (_601 * _488);
        float3 _604 = _603.xyz;
        float3 _617;
        switch (0u)
        {
            default:
            {
                float _607 = _603.x;
                if (_607 == 0.0)
                {
                    _617 = float3(0.0);
                    break;
                }
                _617 = (((_604 * _603.w) / float3(_607)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _724;
        if (_255 > 0.0)
        {
            float3 _723;
            switch (0u)
            {
                default:
                {
                    float _624 = fast::clamp(_526 / _525, -1.0, 1.0);
                    if (_291)
                    {
                        float _673 = -_624;
                        float _684 = _116 - _525;
                        float _697 = -_278;
                        float _708 = _116 - _277;
                        _723 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_525) * _673) + sqrt(fast::max((_530 * ((_673 * _673) - 1.0)) + _205, 0.0)), 0.0) - _684) / ((_533 + _208) - _684)) * 0.99609375), 0.0078125 + (_534 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_277) * _697) + sqrt(fast::max((_285 * ((_697 * _697) - 1.0)) + _205, 0.0)), 0.0) - _708) / ((_423 + _208) - _708)) * 0.99609375), 0.0078125 + (_424 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                    else
                    {
                        float _635 = _116 - _277;
                        float _658 = _116 - _525;
                        _723 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_277) * _278) + sqrt(fast::max(_288 + _205, 0.0)), 0.0) - _635) / ((_423 + _208) - _635)) * 0.99609375), 0.0078125 + (_424 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_525) * _624) + sqrt(fast::max((_530 * ((_624 * _624) - 1.0)) + _205, 0.0)), 0.0) - _658) / ((_533 + _208) - _658)) * 0.99609375), 0.0078125 + (_534 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                }
            }
            _724 = _723;
        }
        else
        {
            _724 = _420;
        }
        float3 _726 = _505 - (_724 * _604);
        float3 _728 = _518 - (_724 * _617);
        float _729 = _728.x;
        float _730 = _726.x;
        float3 _745;
        switch (0u)
        {
            default:
            {
                if (_730 == 0.0)
                {
                    _745 = float3(0.0);
                    break;
                }
                _745 = (((float4(_730, _726.yz, _729).xyz * _729) / float3(_730)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _749 = 1.0 + (_281 * _281);
        _761 = (((_161.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_236.xyz * smoothstep(_199 * (-0.004674999974668025970458984375), _199 * 0.004674999974668025970458984375, _179 - (-sqrt(fast::max(1.0 - (_199 * _199), 0.0))))) * fast::max(dot(_174, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_192.xyz * (1.0 + (dot(_174, _173) / _177))) * 0.5))) * _420) + ((_726 * (0.0596831031143665313720703125 * _749)) + ((_745 * smoothstep(0.0, 0.00999999977648258209228515625, _280)) * ((0.01627720706164836883544921875 * _749) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _281), 1.5))));
    }
    else
    {
        _761 = float3(0.0);
    }
    float3 _763 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    float _764 = dot(_763, _129);
    float _766 = _764 * _764;
    float _768 = -_764;
    float _769 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _772 = _768 - sqrt(_769 - (dot(_763, _763) - _766));
    bool _773 = _772 > 0.0;
    float3 _1252;
    if (_773)
    {
        float3 _778 = (u_fragParams.u_camera.xyz + (_129 * _772)) - u_fragParams.u_earthCenter.xyz;
        float3 _779 = normalize(_778);
        float _782 = length(_778);
        float _784 = dot(_778, u_fragParams.u_sunDirection.xyz) / _782;
        float _786 = _116 - u_fragParams.u_earthCenter.w;
        float4 _797 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_784 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_782 - u_fragParams.u_earthCenter.w) / _786) * 0.9375)));
        float _804 = u_fragParams.u_earthCenter.w / _782;
        float _810 = _116 * _116;
        float _812 = sqrt(_810 - _769);
        float _813 = _782 * _782;
        float _816 = sqrt(fast::max(_813 - _769, 0.0));
        float _827 = _116 - _782;
        float4 _840 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_782) * _784) + sqrt(fast::max((_813 * ((_784 * _784) - 1.0)) + _810, 0.0)), 0.0) - _827) / ((_816 + _812) - _827)) * 0.99609375), 0.0078125 + ((_816 / _812) * 0.984375)));
        float3 _858 = normalize(_778 - _763);
        float _859 = length(_763);
        float _860 = dot(_763, _858);
        float _867 = (-_860) - sqrt(((_860 * _860) - (_859 * _859)) + _810);
        bool _868 = _867 > 0.0;
        float3 _874;
        float _875;
        if (_868)
        {
            _874 = _763 + (_858 * _867);
            _875 = _860 + _867;
        }
        else
        {
            _874 = _763;
            _875 = _860;
        }
        float _894;
        float _876 = _868 ? _116 : _859;
        float _877 = _875 / _876;
        float _878 = dot(_874, u_fragParams.u_sunDirection.xyz);
        float _879 = _878 / _876;
        float _880 = dot(_858, u_fragParams.u_sunDirection.xyz);
        float _882 = length(_778 - _874);
        float _884 = _876 * _876;
        float _887 = _884 * ((_877 * _877) - 1.0);
        bool _890 = (_877 < 0.0) && ((_887 + _769) >= 0.0);
        float3 _1019;
        switch (0u)
        {
            default:
            {
                _894 = (2.0 * _876) * _877;
                float _899 = fast::clamp(sqrt((_882 * (_882 + _894)) + _884), u_fragParams.u_earthCenter.w, _116);
                float _902 = fast::clamp((_875 + _882) / _899, -1.0, 1.0);
                if (_890)
                {
                    float _960 = -_902;
                    float _961 = _899 * _899;
                    float _964 = sqrt(fast::max(_961 - _769, 0.0));
                    float _975 = _116 - _899;
                    float _989 = -_877;
                    float _992 = sqrt(fast::max(_884 - _769, 0.0));
                    float _1003 = _116 - _876;
                    _1019 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_899) * _960) + sqrt(fast::max((_961 * ((_960 * _960) - 1.0)) + _810, 0.0)), 0.0) - _975) / ((_964 + _812) - _975)) * 0.99609375), 0.0078125 + ((_964 / _812) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_876) * _989) + sqrt(fast::max((_884 * ((_989 * _989) - 1.0)) + _810, 0.0)), 0.0) - _1003) / ((_992 + _812) - _1003)) * 0.99609375), 0.0078125 + ((_992 / _812) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _908 = sqrt(fast::max(_884 - _769, 0.0));
                    float _916 = _116 - _876;
                    float _930 = _899 * _899;
                    float _933 = sqrt(fast::max(_930 - _769, 0.0));
                    float _944 = _116 - _899;
                    _1019 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_876) * _877) + sqrt(fast::max(_887 + _810, 0.0)), 0.0) - _916) / ((_908 + _812) - _916)) * 0.99609375), 0.0078125 + ((_908 / _812) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_899) * _902) + sqrt(fast::max((_930 * ((_902 * _902) - 1.0)) + _810, 0.0)), 0.0) - _944) / ((_933 + _812) - _944)) * 0.99609375), 0.0078125 + ((_933 / _812) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _1022 = sqrt(fast::max(_884 - _769, 0.0));
        float _1025 = 0.015625 + ((_1022 / _812) * 0.96875);
        float _1028 = ((_875 * _875) - _884) + _769;
        float _1061;
        if (_890)
        {
            float _1051 = _876 - u_fragParams.u_earthCenter.w;
            _1061 = 0.5 - (0.5 * (0.0078125 + (((_1022 == _1051) ? 0.0 : ((((-_875) - sqrt(fast::max(_1028, 0.0))) - _1051) / (_1022 - _1051))) * 0.984375)));
        }
        else
        {
            float _1038 = _116 - _876;
            _1061 = 0.5 + (0.5 * (0.0078125 + (((((-_875) + sqrt(fast::max(_1028 + (_812 * _812), 0.0))) - _1038) / ((_1022 + _812) - _1038)) * 0.984375)));
        }
        float _1066 = -u_fragParams.u_earthCenter.w;
        float _1073 = _812 - _786;
        float _1074 = (fast::max((_1066 * _879) + sqrt(fast::max((_769 * ((_879 * _879) - 1.0)) + _810, 0.0)), 0.0) - _786) / _1073;
        float _1076 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1073;
        float _1083 = 0.015625 + ((fast::max(1.0 - (_1074 / _1076), 0.0) / (1.0 + _1074)) * 0.96875);
        float _1085 = (_880 + 1.0) * 3.5;
        float _1086 = floor(_1085);
        float _1087 = _1085 - _1086;
        float _1091 = _1086 + 1.0;
        float4 _1097 = scatteringTexture.sample(scatteringSampler, float3((_1086 + _1083) * 0.125, _1061, _1025));
        float _1098 = 1.0 - _1087;
        float4 _1101 = scatteringTexture.sample(scatteringSampler, float3((_1091 + _1083) * 0.125, _1061, _1025));
        float4 _1103 = (_1097 * _1098) + (_1101 * _1087);
        float3 _1104 = _1103.xyz;
        float3 _1117;
        switch (0u)
        {
            default:
            {
                float _1107 = _1103.x;
                if (_1107 == 0.0)
                {
                    _1117 = float3(0.0);
                    break;
                }
                _1117 = (((_1104 * _1103.w) / float3(_1107)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1118 = fast::max(_882, 0.0);
        float _1123 = fast::clamp(sqrt((_1118 * (_1118 + _894)) + _884), u_fragParams.u_earthCenter.w, _116);
        float _1124 = _875 + _1118;
        float _1127 = (_878 + (_1118 * _880)) / _1123;
        float _1128 = _1123 * _1123;
        float _1131 = sqrt(fast::max(_1128 - _769, 0.0));
        float _1134 = 0.015625 + ((_1131 / _812) * 0.96875);
        float _1137 = ((_1124 * _1124) - _1128) + _769;
        float _1170;
        if (_890)
        {
            float _1160 = _1123 - u_fragParams.u_earthCenter.w;
            _1170 = 0.5 - (0.5 * (0.0078125 + (((_1131 == _1160) ? 0.0 : ((((-_1124) - sqrt(fast::max(_1137, 0.0))) - _1160) / (_1131 - _1160))) * 0.984375)));
        }
        else
        {
            float _1147 = _116 - _1123;
            _1170 = 0.5 + (0.5 * (0.0078125 + (((((-_1124) + sqrt(fast::max(_1137 + (_812 * _812), 0.0))) - _1147) / ((_1131 + _812) - _1147)) * 0.984375)));
        }
        float _1181 = (fast::max((_1066 * _1127) + sqrt(fast::max((_769 * ((_1127 * _1127) - 1.0)) + _810, 0.0)), 0.0) - _786) / _1073;
        float _1188 = 0.015625 + ((fast::max(1.0 - (_1181 / _1076), 0.0) / (1.0 + _1181)) * 0.96875);
        float4 _1196 = scatteringTexture.sample(scatteringSampler, float3((_1086 + _1188) * 0.125, _1170, _1134));
        float4 _1199 = scatteringTexture.sample(scatteringSampler, float3((_1091 + _1188) * 0.125, _1170, _1134));
        float4 _1201 = (_1196 * _1098) + (_1199 * _1087);
        float3 _1202 = _1201.xyz;
        float3 _1215;
        switch (0u)
        {
            default:
            {
                float _1205 = _1201.x;
                if (_1205 == 0.0)
                {
                    _1215 = float3(0.0);
                    break;
                }
                _1215 = (((_1202 * _1201.w) / float3(_1205)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _1217 = _1104 - (_1019 * _1202);
        float3 _1219 = _1117 - (_1019 * _1215);
        float _1220 = _1219.x;
        float _1221 = _1217.x;
        float3 _1236;
        switch (0u)
        {
            default:
            {
                if (_1221 == 0.0)
                {
                    _1236 = float3(0.0);
                    break;
                }
                _1236 = (((float4(_1221, _1217.yz, _1220).xyz * _1220) / float3(_1221)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1240 = 1.0 + (_880 * _880);
        _1252 = (((_161.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_840.xyz * smoothstep(_804 * (-0.004674999974668025970458984375), _804 * 0.004674999974668025970458984375, _784 - (-sqrt(fast::max(1.0 - (_804 * _804), 0.0))))) * fast::max(dot(_779, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_797.xyz * (1.0 + (dot(_779, _778) / _782))) * 0.5))) * _1019) + ((_1217 * (0.0596831031143665313720703125 * _1240)) + ((_1236 * smoothstep(0.0, 0.00999999977648258209228515625, _879)) * ((0.01627720706164836883544921875 * _1240) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _880), 1.5))));
    }
    else
    {
        _1252 = float3(0.0);
    }
    float3 _1422;
    float3 _1423;
    switch (0u)
    {
        default:
        {
            float _1258 = length(_763);
            float _1261 = _116 * _116;
            float _1264 = _768 - sqrt((_766 - (_1258 * _1258)) + _1261);
            bool _1265 = _1264 > 0.0;
            float3 _1275;
            float _1276;
            if (_1265)
            {
                _1275 = _763 + (_129 * _1264);
                _1276 = _764 + _1264;
            }
            else
            {
                if (_1258 > _116)
                {
                    _1422 = float3(1.0);
                    _1423 = float3(0.0);
                    break;
                }
                _1275 = _763;
                _1276 = _764;
            }
            float _1277 = _1265 ? _116 : _1258;
            float _1278 = _1276 / _1277;
            float _1280 = dot(_1275, u_fragParams.u_sunDirection.xyz) / _1277;
            float _1281 = dot(_129, u_fragParams.u_sunDirection.xyz);
            float _1283 = _1277 * _1277;
            float _1286 = _1283 * ((_1278 * _1278) - 1.0);
            bool _1289 = (_1278 < 0.0) && ((_1286 + _769) >= 0.0);
            float _1291 = sqrt(_1261 - _769);
            float _1294 = sqrt(fast::max(_1283 - _769, 0.0));
            float _1302 = _116 - _1277;
            float _1305 = (_1294 + _1291) - _1302;
            float _1307 = _1294 / _1291;
            float4 _1315 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_1277) * _1278) + sqrt(fast::max(_1286 + _1261, 0.0)), 0.0) - _1302) / _1305) * 0.99609375), 0.0078125 + (_1307 * 0.984375)));
            float _1320 = 0.015625 + (_1307 * 0.96875);
            float _1323 = ((_1276 * _1276) - _1283) + _769;
            float _1353;
            if (_1289)
            {
                float _1343 = _1277 - u_fragParams.u_earthCenter.w;
                _1353 = 0.5 - (0.5 * (0.0078125 + (((_1294 == _1343) ? 0.0 : ((((-_1276) - sqrt(fast::max(_1323, 0.0))) - _1343) / (_1294 - _1343))) * 0.984375)));
            }
            else
            {
                _1353 = 0.5 + (0.5 * (0.0078125 + (((((-_1276) + sqrt(fast::max(_1323 + (_1291 * _1291), 0.0))) - _1302) / _1305) * 0.984375)));
            }
            float _1364 = _116 - u_fragParams.u_earthCenter.w;
            float _1366 = _1291 - _1364;
            float _1367 = (fast::max(((-u_fragParams.u_earthCenter.w) * _1280) + sqrt(fast::max((_769 * ((_1280 * _1280) - 1.0)) + _1261, 0.0)), 0.0) - _1364) / _1366;
            float _1376 = 0.015625 + ((fast::max(1.0 - (_1367 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1366)), 0.0) / (1.0 + _1367)) * 0.96875);
            float _1378 = (_1281 + 1.0) * 3.5;
            float _1379 = floor(_1378);
            float _1380 = _1378 - _1379;
            float4 _1390 = scatteringTexture.sample(scatteringSampler, float3((_1379 + _1376) * 0.125, _1353, _1320));
            float4 _1394 = scatteringTexture.sample(scatteringSampler, float3(((_1379 + 1.0) + _1376) * 0.125, _1353, _1320));
            float4 _1396 = (_1390 * (1.0 - _1380)) + (_1394 * _1380);
            float3 _1397 = _1396.xyz;
            float3 _1410;
            switch (0u)
            {
                default:
                {
                    float _1400 = _1396.x;
                    if (_1400 == 0.0)
                    {
                        _1410 = float3(0.0);
                        break;
                    }
                    _1410 = (((_1397 * _1396.w) / float3(_1400)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1412 = 1.0 + (_1281 * _1281);
            _1422 = select(_1315.xyz, float3(0.0), bool3(_1289));
            _1423 = (_1397 * (0.0596831031143665313720703125 * _1412)) + (_1410 * ((0.01627720706164836883544921875 * _1412) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1281), 1.5)));
            break;
        }
    }
    float3 _1431;
    if (dot(_129, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1431 = _1423 + (_1422 * float3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1431 = _1423;
    }
    float3 _1445 = pow(abs(float3(1.0) - exp(((-mix(mix(_1431, _1252, float3(float(_773))), _761, float3(float(_170)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), float3(0.4545454680919647216796875));
    float4 _1447 = float4(_1445.x, _1445.y, _1445.z, _108.w);
    _1447.w = 1.0;
    out.out_var_SV_Target0 = _1447;
    out.out_var_SV_Target1 = _137;
    out.gl_FragDepth = _142;
    return out;
}

