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
    float4 u_earthNorth;
};

constant float4 _110 = {};

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
    float _118 = u_fragParams.u_earthCenter.w + 60000.0;
    float3 _131 = normalize(in.in_var_TEXCOORD1);
    float4 _135 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0);
    float4 _139 = sceneNormalTexture.sample(sceneNormalSampler, in.in_var_TEXCOORD0);
    float4 _143 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _144 = _143.x;
    float _149 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float2 _167 = _139.zw;
    float3 _172 = float3(_139.zw, float(int(sign(_139.y))) * sqrt(1.0 - dot(_167, _167)));
    float3 _175 = pow(abs(_135.xyz), float3(2.2000000476837158203125));
    float3 _179 = select(_172, float3(0.0, 0.0, 1.0), bool3(length(_172) == 0.0));
    float _185 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _149) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _144 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _149))) * u_cameraPlaneParams.s_CameraFarPlane;
    float3 _187 = u_fragParams.u_camera.xyz + (_131 * _185);
    float3 _188 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    float _189 = dot(_188, _131);
    float _191 = _189 * _189;
    float _193 = -_189;
    float _194 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _197 = _193 - sqrt(_194 - (dot(_188, _188) - _191));
    bool _198 = _197 > 0.0;
    float3 _203;
    if (_198)
    {
        _203 = u_fragParams.u_camera.xyz + (_131 * _197);
    }
    else
    {
        _203 = _187;
    }
    float3 _207 = _203 - u_fragParams.u_earthCenter.xyz;
    float3 _208 = normalize(_207);
    float3 _217 = _179;
    _217.y = _179.y * (-1.0);
    float3 _218 = float3x3(normalize(cross(u_fragParams.u_earthNorth.xyz, _208)), u_fragParams.u_earthNorth.xyz, _208) * _217;
    bool _219 = _144 < 0.75;
    float3 _807;
    if (_219)
    {
        float3 _222 = _187 - u_fragParams.u_earthCenter.xyz;
        float _225 = length(_222);
        float _227 = dot(_222, u_fragParams.u_sunDirection.xyz) / _225;
        float _229 = _118 - u_fragParams.u_earthCenter.w;
        float4 _240 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_227 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_225 - u_fragParams.u_earthCenter.w) / _229) * 0.9375)));
        float _247 = u_fragParams.u_earthCenter.w / _225;
        float _253 = _118 * _118;
        float _255 = sqrt(_253 - _194);
        float _256 = _225 * _225;
        float _259 = sqrt(fast::max(_256 - _194, 0.0));
        float _270 = _118 - _225;
        float4 _283 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_225) * _227) + sqrt(fast::max((_256 * ((_227 * _227) - 1.0)) + _253, 0.0)), 0.0) - _270) / ((_259 + _255) - _270)) * 0.99609375), 0.0078125 + ((_259 / _255) * 0.984375)));
        float _300 = fast::max(0.0, fast::min(0.0, _185));
        float3 _303 = normalize(_222 - _188);
        float _304 = length(_188);
        float _305 = dot(_188, _303);
        float _312 = (-_305) - sqrt(((_305 * _305) - (_304 * _304)) + _253);
        bool _313 = _312 > 0.0;
        float3 _319;
        float _320;
        if (_313)
        {
            _319 = _188 + (_303 * _312);
            _320 = _305 + _312;
        }
        else
        {
            _319 = _188;
            _320 = _305;
        }
        float _339;
        float _321 = _313 ? _118 : _304;
        float _322 = _320 / _321;
        float _323 = dot(_319, u_fragParams.u_sunDirection.xyz);
        float _324 = _323 / _321;
        float _325 = dot(_303, u_fragParams.u_sunDirection.xyz);
        float _327 = length(_222 - _319);
        float _329 = _321 * _321;
        float _332 = _329 * ((_322 * _322) - 1.0);
        bool _335 = (_322 < 0.0) && ((_332 + _194) >= 0.0);
        float3 _464;
        switch (0u)
        {
            default:
            {
                _339 = (2.0 * _321) * _322;
                float _344 = fast::clamp(sqrt((_327 * (_327 + _339)) + _329), u_fragParams.u_earthCenter.w, _118);
                float _347 = fast::clamp((_320 + _327) / _344, -1.0, 1.0);
                if (_335)
                {
                    float _405 = -_347;
                    float _406 = _344 * _344;
                    float _409 = sqrt(fast::max(_406 - _194, 0.0));
                    float _420 = _118 - _344;
                    float _434 = -_322;
                    float _437 = sqrt(fast::max(_329 - _194, 0.0));
                    float _448 = _118 - _321;
                    _464 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_344) * _405) + sqrt(fast::max((_406 * ((_405 * _405) - 1.0)) + _253, 0.0)), 0.0) - _420) / ((_409 + _255) - _420)) * 0.99609375), 0.0078125 + ((_409 / _255) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_321) * _434) + sqrt(fast::max((_329 * ((_434 * _434) - 1.0)) + _253, 0.0)), 0.0) - _448) / ((_437 + _255) - _448)) * 0.99609375), 0.0078125 + ((_437 / _255) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _353 = sqrt(fast::max(_329 - _194, 0.0));
                    float _361 = _118 - _321;
                    float _375 = _344 * _344;
                    float _378 = sqrt(fast::max(_375 - _194, 0.0));
                    float _389 = _118 - _344;
                    _464 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_321) * _322) + sqrt(fast::max(_332 + _253, 0.0)), 0.0) - _361) / ((_353 + _255) - _361)) * 0.99609375), 0.0078125 + ((_353 / _255) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_344) * _347) + sqrt(fast::max((_375 * ((_347 * _347) - 1.0)) + _253, 0.0)), 0.0) - _389) / ((_378 + _255) - _389)) * 0.99609375), 0.0078125 + ((_378 / _255) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _467 = sqrt(fast::max(_329 - _194, 0.0));
        float _468 = _467 / _255;
        float _470 = 0.015625 + (_468 * 0.96875);
        float _473 = ((_320 * _320) - _329) + _194;
        float _506;
        if (_335)
        {
            float _496 = _321 - u_fragParams.u_earthCenter.w;
            _506 = 0.5 - (0.5 * (0.0078125 + (((_467 == _496) ? 0.0 : ((((-_320) - sqrt(fast::max(_473, 0.0))) - _496) / (_467 - _496))) * 0.984375)));
        }
        else
        {
            float _483 = _118 - _321;
            _506 = 0.5 + (0.5 * (0.0078125 + (((((-_320) + sqrt(fast::max(_473 + (_255 * _255), 0.0))) - _483) / ((_467 + _255) - _483)) * 0.984375)));
        }
        float _511 = -u_fragParams.u_earthCenter.w;
        float _518 = _255 - _229;
        float _519 = (fast::max((_511 * _324) + sqrt(fast::max((_194 * ((_324 * _324) - 1.0)) + _253, 0.0)), 0.0) - _229) / _518;
        float _521 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _518;
        float _528 = 0.015625 + ((fast::max(1.0 - (_519 / _521), 0.0) / (1.0 + _519)) * 0.96875);
        float _530 = (_325 + 1.0) * 3.5;
        float _531 = floor(_530);
        float _532 = _530 - _531;
        float _536 = _531 + 1.0;
        float4 _542 = scatteringTexture.sample(scatteringSampler, float3((_531 + _528) * 0.125, _506, _470));
        float _543 = 1.0 - _532;
        float4 _546 = scatteringTexture.sample(scatteringSampler, float3((_536 + _528) * 0.125, _506, _470));
        float4 _548 = (_542 * _543) + (_546 * _532);
        float3 _549 = _548.xyz;
        float3 _562;
        switch (0u)
        {
            default:
            {
                float _552 = _548.x;
                if (_552 == 0.0)
                {
                    _562 = float3(0.0);
                    break;
                }
                _562 = (((_549 * _548.w) / float3(_552)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _564 = fast::max(_327 - _300, 0.0);
        float _569 = fast::clamp(sqrt((_564 * (_564 + _339)) + _329), u_fragParams.u_earthCenter.w, _118);
        float _570 = _320 + _564;
        float _573 = (_323 + (_564 * _325)) / _569;
        float _574 = _569 * _569;
        float _577 = sqrt(fast::max(_574 - _194, 0.0));
        float _578 = _577 / _255;
        float _580 = 0.015625 + (_578 * 0.96875);
        float _583 = ((_570 * _570) - _574) + _194;
        float _616;
        if (_335)
        {
            float _606 = _569 - u_fragParams.u_earthCenter.w;
            _616 = 0.5 - (0.5 * (0.0078125 + (((_577 == _606) ? 0.0 : ((((-_570) - sqrt(fast::max(_583, 0.0))) - _606) / (_577 - _606))) * 0.984375)));
        }
        else
        {
            float _593 = _118 - _569;
            _616 = 0.5 + (0.5 * (0.0078125 + (((((-_570) + sqrt(fast::max(_583 + (_255 * _255), 0.0))) - _593) / ((_577 + _255) - _593)) * 0.984375)));
        }
        float _627 = (fast::max((_511 * _573) + sqrt(fast::max((_194 * ((_573 * _573) - 1.0)) + _253, 0.0)), 0.0) - _229) / _518;
        float _634 = 0.015625 + ((fast::max(1.0 - (_627 / _521), 0.0) / (1.0 + _627)) * 0.96875);
        float4 _642 = scatteringTexture.sample(scatteringSampler, float3((_531 + _634) * 0.125, _616, _580));
        float4 _645 = scatteringTexture.sample(scatteringSampler, float3((_536 + _634) * 0.125, _616, _580));
        float4 _647 = (_642 * _543) + (_645 * _532);
        float3 _648 = _647.xyz;
        float3 _661;
        switch (0u)
        {
            default:
            {
                float _651 = _647.x;
                if (_651 == 0.0)
                {
                    _661 = float3(0.0);
                    break;
                }
                _661 = (((_648 * _647.w) / float3(_651)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _768;
        if (_300 > 0.0)
        {
            float3 _767;
            switch (0u)
            {
                default:
                {
                    float _668 = fast::clamp(_570 / _569, -1.0, 1.0);
                    if (_335)
                    {
                        float _717 = -_668;
                        float _728 = _118 - _569;
                        float _741 = -_322;
                        float _752 = _118 - _321;
                        _767 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_569) * _717) + sqrt(fast::max((_574 * ((_717 * _717) - 1.0)) + _253, 0.0)), 0.0) - _728) / ((_577 + _255) - _728)) * 0.99609375), 0.0078125 + (_578 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_321) * _741) + sqrt(fast::max((_329 * ((_741 * _741) - 1.0)) + _253, 0.0)), 0.0) - _752) / ((_467 + _255) - _752)) * 0.99609375), 0.0078125 + (_468 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                    else
                    {
                        float _679 = _118 - _321;
                        float _702 = _118 - _569;
                        _767 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_321) * _322) + sqrt(fast::max(_332 + _253, 0.0)), 0.0) - _679) / ((_467 + _255) - _679)) * 0.99609375), 0.0078125 + (_468 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_569) * _668) + sqrt(fast::max((_574 * ((_668 * _668) - 1.0)) + _253, 0.0)), 0.0) - _702) / ((_577 + _255) - _702)) * 0.99609375), 0.0078125 + (_578 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                }
            }
            _768 = _767;
        }
        else
        {
            _768 = _464;
        }
        float3 _770 = _549 - (_768 * _648);
        float3 _772 = _562 - (_768 * _661);
        float _773 = _772.x;
        float _774 = _770.x;
        float3 _789;
        switch (0u)
        {
            default:
            {
                if (_774 == 0.0)
                {
                    _789 = float3(0.0);
                    break;
                }
                _789 = (((float4(_774, _770.yz, _773).xyz * _773) / float3(_774)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _793 = 1.0 + (_325 * _325);
        _807 = (((_175.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_283.xyz * smoothstep(_247 * (-0.004674999974668025970458984375), _247 * 0.004674999974668025970458984375, _227 - (-sqrt(fast::max(1.0 - (_247 * _247), 0.0))))) * fast::max(dot(_218, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_240.xyz * (1.0 + (dot(_218, _222) / _225))) * 0.5))) * _464) + (((_770 * (0.0596831031143665313720703125 * _793)) + ((_789 * smoothstep(0.0, 0.00999999977648258209228515625, _324)) * ((0.01627720706164836883544921875 * _793) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _325), 1.5)))) * mix(0.5, 1.0, fast::min(1.0, pow(_144, 6.0) * 6.0)));
    }
    else
    {
        _807 = float3(0.0);
    }
    float3 _1283;
    if (_198)
    {
        float _813 = length(_207);
        float _815 = dot(_207, u_fragParams.u_sunDirection.xyz) / _813;
        float _817 = _118 - u_fragParams.u_earthCenter.w;
        float4 _828 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_815 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_813 - u_fragParams.u_earthCenter.w) / _817) * 0.9375)));
        float _835 = u_fragParams.u_earthCenter.w / _813;
        float _841 = _118 * _118;
        float _843 = sqrt(_841 - _194);
        float _844 = _813 * _813;
        float _847 = sqrt(fast::max(_844 - _194, 0.0));
        float _858 = _118 - _813;
        float4 _871 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_813) * _815) + sqrt(fast::max((_844 * ((_815 * _815) - 1.0)) + _841, 0.0)), 0.0) - _858) / ((_847 + _843) - _858)) * 0.99609375), 0.0078125 + ((_847 / _843) * 0.984375)));
        float3 _889 = normalize(_207 - _188);
        float _890 = length(_188);
        float _891 = dot(_188, _889);
        float _898 = (-_891) - sqrt(((_891 * _891) - (_890 * _890)) + _841);
        bool _899 = _898 > 0.0;
        float3 _905;
        float _906;
        if (_899)
        {
            _905 = _188 + (_889 * _898);
            _906 = _891 + _898;
        }
        else
        {
            _905 = _188;
            _906 = _891;
        }
        float _925;
        float _907 = _899 ? _118 : _890;
        float _908 = _906 / _907;
        float _909 = dot(_905, u_fragParams.u_sunDirection.xyz);
        float _910 = _909 / _907;
        float _911 = dot(_889, u_fragParams.u_sunDirection.xyz);
        float _913 = length(_207 - _905);
        float _915 = _907 * _907;
        float _918 = _915 * ((_908 * _908) - 1.0);
        bool _921 = (_908 < 0.0) && ((_918 + _194) >= 0.0);
        float3 _1050;
        switch (0u)
        {
            default:
            {
                _925 = (2.0 * _907) * _908;
                float _930 = fast::clamp(sqrt((_913 * (_913 + _925)) + _915), u_fragParams.u_earthCenter.w, _118);
                float _933 = fast::clamp((_906 + _913) / _930, -1.0, 1.0);
                if (_921)
                {
                    float _991 = -_933;
                    float _992 = _930 * _930;
                    float _995 = sqrt(fast::max(_992 - _194, 0.0));
                    float _1006 = _118 - _930;
                    float _1020 = -_908;
                    float _1023 = sqrt(fast::max(_915 - _194, 0.0));
                    float _1034 = _118 - _907;
                    _1050 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_930) * _991) + sqrt(fast::max((_992 * ((_991 * _991) - 1.0)) + _841, 0.0)), 0.0) - _1006) / ((_995 + _843) - _1006)) * 0.99609375), 0.0078125 + ((_995 / _843) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_907) * _1020) + sqrt(fast::max((_915 * ((_1020 * _1020) - 1.0)) + _841, 0.0)), 0.0) - _1034) / ((_1023 + _843) - _1034)) * 0.99609375), 0.0078125 + ((_1023 / _843) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _939 = sqrt(fast::max(_915 - _194, 0.0));
                    float _947 = _118 - _907;
                    float _961 = _930 * _930;
                    float _964 = sqrt(fast::max(_961 - _194, 0.0));
                    float _975 = _118 - _930;
                    _1050 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_907) * _908) + sqrt(fast::max(_918 + _841, 0.0)), 0.0) - _947) / ((_939 + _843) - _947)) * 0.99609375), 0.0078125 + ((_939 / _843) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_930) * _933) + sqrt(fast::max((_961 * ((_933 * _933) - 1.0)) + _841, 0.0)), 0.0) - _975) / ((_964 + _843) - _975)) * 0.99609375), 0.0078125 + ((_964 / _843) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _1053 = sqrt(fast::max(_915 - _194, 0.0));
        float _1056 = 0.015625 + ((_1053 / _843) * 0.96875);
        float _1059 = ((_906 * _906) - _915) + _194;
        float _1092;
        if (_921)
        {
            float _1082 = _907 - u_fragParams.u_earthCenter.w;
            _1092 = 0.5 - (0.5 * (0.0078125 + (((_1053 == _1082) ? 0.0 : ((((-_906) - sqrt(fast::max(_1059, 0.0))) - _1082) / (_1053 - _1082))) * 0.984375)));
        }
        else
        {
            float _1069 = _118 - _907;
            _1092 = 0.5 + (0.5 * (0.0078125 + (((((-_906) + sqrt(fast::max(_1059 + (_843 * _843), 0.0))) - _1069) / ((_1053 + _843) - _1069)) * 0.984375)));
        }
        float _1097 = -u_fragParams.u_earthCenter.w;
        float _1104 = _843 - _817;
        float _1105 = (fast::max((_1097 * _910) + sqrt(fast::max((_194 * ((_910 * _910) - 1.0)) + _841, 0.0)), 0.0) - _817) / _1104;
        float _1107 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1104;
        float _1114 = 0.015625 + ((fast::max(1.0 - (_1105 / _1107), 0.0) / (1.0 + _1105)) * 0.96875);
        float _1116 = (_911 + 1.0) * 3.5;
        float _1117 = floor(_1116);
        float _1118 = _1116 - _1117;
        float _1122 = _1117 + 1.0;
        float4 _1128 = scatteringTexture.sample(scatteringSampler, float3((_1117 + _1114) * 0.125, _1092, _1056));
        float _1129 = 1.0 - _1118;
        float4 _1132 = scatteringTexture.sample(scatteringSampler, float3((_1122 + _1114) * 0.125, _1092, _1056));
        float4 _1134 = (_1128 * _1129) + (_1132 * _1118);
        float3 _1135 = _1134.xyz;
        float3 _1148;
        switch (0u)
        {
            default:
            {
                float _1138 = _1134.x;
                if (_1138 == 0.0)
                {
                    _1148 = float3(0.0);
                    break;
                }
                _1148 = (((_1135 * _1134.w) / float3(_1138)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1149 = fast::max(_913, 0.0);
        float _1154 = fast::clamp(sqrt((_1149 * (_1149 + _925)) + _915), u_fragParams.u_earthCenter.w, _118);
        float _1155 = _906 + _1149;
        float _1158 = (_909 + (_1149 * _911)) / _1154;
        float _1159 = _1154 * _1154;
        float _1162 = sqrt(fast::max(_1159 - _194, 0.0));
        float _1165 = 0.015625 + ((_1162 / _843) * 0.96875);
        float _1168 = ((_1155 * _1155) - _1159) + _194;
        float _1201;
        if (_921)
        {
            float _1191 = _1154 - u_fragParams.u_earthCenter.w;
            _1201 = 0.5 - (0.5 * (0.0078125 + (((_1162 == _1191) ? 0.0 : ((((-_1155) - sqrt(fast::max(_1168, 0.0))) - _1191) / (_1162 - _1191))) * 0.984375)));
        }
        else
        {
            float _1178 = _118 - _1154;
            _1201 = 0.5 + (0.5 * (0.0078125 + (((((-_1155) + sqrt(fast::max(_1168 + (_843 * _843), 0.0))) - _1178) / ((_1162 + _843) - _1178)) * 0.984375)));
        }
        float _1212 = (fast::max((_1097 * _1158) + sqrt(fast::max((_194 * ((_1158 * _1158) - 1.0)) + _841, 0.0)), 0.0) - _817) / _1104;
        float _1219 = 0.015625 + ((fast::max(1.0 - (_1212 / _1107), 0.0) / (1.0 + _1212)) * 0.96875);
        float4 _1227 = scatteringTexture.sample(scatteringSampler, float3((_1117 + _1219) * 0.125, _1201, _1165));
        float4 _1230 = scatteringTexture.sample(scatteringSampler, float3((_1122 + _1219) * 0.125, _1201, _1165));
        float4 _1232 = (_1227 * _1129) + (_1230 * _1118);
        float3 _1233 = _1232.xyz;
        float3 _1246;
        switch (0u)
        {
            default:
            {
                float _1236 = _1232.x;
                if (_1236 == 0.0)
                {
                    _1246 = float3(0.0);
                    break;
                }
                _1246 = (((_1233 * _1232.w) / float3(_1236)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _1248 = _1135 - (_1050 * _1233);
        float3 _1250 = _1148 - (_1050 * _1246);
        float _1251 = _1250.x;
        float _1252 = _1248.x;
        float3 _1267;
        switch (0u)
        {
            default:
            {
                if (_1252 == 0.0)
                {
                    _1267 = float3(0.0);
                    break;
                }
                _1267 = (((float4(_1252, _1248.yz, _1251).xyz * _1251) / float3(_1252)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1271 = 1.0 + (_911 * _911);
        _1283 = (((_175.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_871.xyz * smoothstep(_835 * (-0.004674999974668025970458984375), _835 * 0.004674999974668025970458984375, _815 - (-sqrt(fast::max(1.0 - (_835 * _835), 0.0))))) * fast::max(dot(_218, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_828.xyz * (1.0 + (dot(_218, _207) / _813))) * 0.5))) * _1050) + ((_1248 * (0.0596831031143665313720703125 * _1271)) + ((_1267 * smoothstep(0.0, 0.00999999977648258209228515625, _910)) * ((0.01627720706164836883544921875 * _1271) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _911), 1.5))));
    }
    else
    {
        _1283 = float3(0.0);
    }
    float3 _1453;
    float3 _1454;
    switch (0u)
    {
        default:
        {
            float _1289 = length(_188);
            float _1292 = _118 * _118;
            float _1295 = _193 - sqrt((_191 - (_1289 * _1289)) + _1292);
            bool _1296 = _1295 > 0.0;
            float3 _1306;
            float _1307;
            if (_1296)
            {
                _1306 = _188 + (_131 * _1295);
                _1307 = _189 + _1295;
            }
            else
            {
                if (_1289 > _118)
                {
                    _1453 = float3(1.0);
                    _1454 = float3(0.0);
                    break;
                }
                _1306 = _188;
                _1307 = _189;
            }
            float _1308 = _1296 ? _118 : _1289;
            float _1309 = _1307 / _1308;
            float _1311 = dot(_1306, u_fragParams.u_sunDirection.xyz) / _1308;
            float _1312 = dot(_131, u_fragParams.u_sunDirection.xyz);
            float _1314 = _1308 * _1308;
            float _1317 = _1314 * ((_1309 * _1309) - 1.0);
            bool _1320 = (_1309 < 0.0) && ((_1317 + _194) >= 0.0);
            float _1322 = sqrt(_1292 - _194);
            float _1325 = sqrt(fast::max(_1314 - _194, 0.0));
            float _1333 = _118 - _1308;
            float _1336 = (_1325 + _1322) - _1333;
            float _1338 = _1325 / _1322;
            float4 _1346 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_1308) * _1309) + sqrt(fast::max(_1317 + _1292, 0.0)), 0.0) - _1333) / _1336) * 0.99609375), 0.0078125 + (_1338 * 0.984375)));
            float _1351 = 0.015625 + (_1338 * 0.96875);
            float _1354 = ((_1307 * _1307) - _1314) + _194;
            float _1384;
            if (_1320)
            {
                float _1374 = _1308 - u_fragParams.u_earthCenter.w;
                _1384 = 0.5 - (0.5 * (0.0078125 + (((_1325 == _1374) ? 0.0 : ((((-_1307) - sqrt(fast::max(_1354, 0.0))) - _1374) / (_1325 - _1374))) * 0.984375)));
            }
            else
            {
                _1384 = 0.5 + (0.5 * (0.0078125 + (((((-_1307) + sqrt(fast::max(_1354 + (_1322 * _1322), 0.0))) - _1333) / _1336) * 0.984375)));
            }
            float _1395 = _118 - u_fragParams.u_earthCenter.w;
            float _1397 = _1322 - _1395;
            float _1398 = (fast::max(((-u_fragParams.u_earthCenter.w) * _1311) + sqrt(fast::max((_194 * ((_1311 * _1311) - 1.0)) + _1292, 0.0)), 0.0) - _1395) / _1397;
            float _1407 = 0.015625 + ((fast::max(1.0 - (_1398 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1397)), 0.0) / (1.0 + _1398)) * 0.96875);
            float _1409 = (_1312 + 1.0) * 3.5;
            float _1410 = floor(_1409);
            float _1411 = _1409 - _1410;
            float4 _1421 = scatteringTexture.sample(scatteringSampler, float3((_1410 + _1407) * 0.125, _1384, _1351));
            float4 _1425 = scatteringTexture.sample(scatteringSampler, float3(((_1410 + 1.0) + _1407) * 0.125, _1384, _1351));
            float4 _1427 = (_1421 * (1.0 - _1411)) + (_1425 * _1411);
            float3 _1428 = _1427.xyz;
            float3 _1441;
            switch (0u)
            {
                default:
                {
                    float _1431 = _1427.x;
                    if (_1431 == 0.0)
                    {
                        _1441 = float3(0.0);
                        break;
                    }
                    _1441 = (((_1428 * _1427.w) / float3(_1431)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1443 = 1.0 + (_1312 * _1312);
            _1453 = select(_1346.xyz, float3(0.0), bool3(_1320));
            _1454 = (_1428 * (0.0596831031143665313720703125 * _1443)) + (_1441 * ((0.01627720706164836883544921875 * _1443) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1312), 1.5)));
            break;
        }
    }
    float3 _1462;
    if (dot(_131, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1462 = _1454 + (_1453 * float3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1462 = _1454;
    }
    float3 _1480 = pow(abs(float3(1.0) - exp(((-mix(mix(_1462, _1283, float3(float(_198))), _807, float3(float(_219) * fast::min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _144))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), float3(0.4545454680919647216796875));
    float4 _1482 = float4(_1480.x, _1480.y, _1480.z, _110.w);
    _1482.w = 1.0;
    out.out_var_SV_Target0 = _1482;
    out.out_var_SV_Target1 = _139;
    out.gl_FragDepth = _144;
    return out;
}

