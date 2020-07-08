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
    float2 _165 = _137.zw;
    float3 _170 = float3(_137.zw, float(int(sign(_137.y))) * sqrt(1.0 - dot(_165, _165)));
    float3 _173 = pow(abs(_133.xyz), float3(2.2000000476837158203125));
    float3 _177 = select(_170, float3(0.0, 0.0, 1.0), bool3(length(_170) == 0.0));
    float _183 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _147) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _142 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _147))) * u_cameraPlaneParams.s_CameraFarPlane;
    float3 _185 = u_fragParams.u_camera.xyz + (_129 * _183);
    float3 _186 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    float _187 = dot(_186, _129);
    float _189 = _187 * _187;
    float _191 = -_187;
    float _192 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _195 = _191 - sqrt(_192 - (dot(_186, _186) - _189));
    bool _196 = _195 > 0.0;
    float3 _201;
    if (_196)
    {
        _201 = u_fragParams.u_camera.xyz + (_129 * _195);
    }
    else
    {
        _201 = _185;
    }
    bool _205 = _142 < 0.75;
    float3 _793;
    if (_205)
    {
        float3 _208 = _185 - u_fragParams.u_earthCenter.xyz;
        float _211 = length(_208);
        float _213 = dot(_208, u_fragParams.u_sunDirection.xyz) / _211;
        float _215 = _116 - u_fragParams.u_earthCenter.w;
        float4 _226 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_213 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_211 - u_fragParams.u_earthCenter.w) / _215) * 0.9375)));
        float _233 = u_fragParams.u_earthCenter.w / _211;
        float _239 = _116 * _116;
        float _241 = sqrt(_239 - _192);
        float _242 = _211 * _211;
        float _245 = sqrt(fast::max(_242 - _192, 0.0));
        float _256 = _116 - _211;
        float4 _269 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_211) * _213) + sqrt(fast::max((_242 * ((_213 * _213) - 1.0)) + _239, 0.0)), 0.0) - _256) / ((_245 + _241) - _256)) * 0.99609375), 0.0078125 + ((_245 / _241) * 0.984375)));
        float _286 = fast::max(0.0, fast::min(0.0, _183));
        float3 _289 = normalize(_208 - _186);
        float _290 = length(_186);
        float _291 = dot(_186, _289);
        float _298 = (-_291) - sqrt(((_291 * _291) - (_290 * _290)) + _239);
        bool _299 = _298 > 0.0;
        float3 _305;
        float _306;
        if (_299)
        {
            _305 = _186 + (_289 * _298);
            _306 = _291 + _298;
        }
        else
        {
            _305 = _186;
            _306 = _291;
        }
        float _325;
        float _307 = _299 ? _116 : _290;
        float _308 = _306 / _307;
        float _309 = dot(_305, u_fragParams.u_sunDirection.xyz);
        float _310 = _309 / _307;
        float _311 = dot(_289, u_fragParams.u_sunDirection.xyz);
        float _313 = length(_208 - _305);
        float _315 = _307 * _307;
        float _318 = _315 * ((_308 * _308) - 1.0);
        bool _321 = (_308 < 0.0) && ((_318 + _192) >= 0.0);
        float3 _450;
        switch (0u)
        {
            default:
            {
                _325 = (2.0 * _307) * _308;
                float _330 = fast::clamp(sqrt((_313 * (_313 + _325)) + _315), u_fragParams.u_earthCenter.w, _116);
                float _333 = fast::clamp((_306 + _313) / _330, -1.0, 1.0);
                if (_321)
                {
                    float _391 = -_333;
                    float _392 = _330 * _330;
                    float _395 = sqrt(fast::max(_392 - _192, 0.0));
                    float _406 = _116 - _330;
                    float _420 = -_308;
                    float _423 = sqrt(fast::max(_315 - _192, 0.0));
                    float _434 = _116 - _307;
                    _450 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_330) * _391) + sqrt(fast::max((_392 * ((_391 * _391) - 1.0)) + _239, 0.0)), 0.0) - _406) / ((_395 + _241) - _406)) * 0.99609375), 0.0078125 + ((_395 / _241) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_307) * _420) + sqrt(fast::max((_315 * ((_420 * _420) - 1.0)) + _239, 0.0)), 0.0) - _434) / ((_423 + _241) - _434)) * 0.99609375), 0.0078125 + ((_423 / _241) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _339 = sqrt(fast::max(_315 - _192, 0.0));
                    float _347 = _116 - _307;
                    float _361 = _330 * _330;
                    float _364 = sqrt(fast::max(_361 - _192, 0.0));
                    float _375 = _116 - _330;
                    _450 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_307) * _308) + sqrt(fast::max(_318 + _239, 0.0)), 0.0) - _347) / ((_339 + _241) - _347)) * 0.99609375), 0.0078125 + ((_339 / _241) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_330) * _333) + sqrt(fast::max((_361 * ((_333 * _333) - 1.0)) + _239, 0.0)), 0.0) - _375) / ((_364 + _241) - _375)) * 0.99609375), 0.0078125 + ((_364 / _241) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _453 = sqrt(fast::max(_315 - _192, 0.0));
        float _454 = _453 / _241;
        float _456 = 0.015625 + (_454 * 0.96875);
        float _459 = ((_306 * _306) - _315) + _192;
        float _492;
        if (_321)
        {
            float _482 = _307 - u_fragParams.u_earthCenter.w;
            _492 = 0.5 - (0.5 * (0.0078125 + (((_453 == _482) ? 0.0 : ((((-_306) - sqrt(fast::max(_459, 0.0))) - _482) / (_453 - _482))) * 0.984375)));
        }
        else
        {
            float _469 = _116 - _307;
            _492 = 0.5 + (0.5 * (0.0078125 + (((((-_306) + sqrt(fast::max(_459 + (_241 * _241), 0.0))) - _469) / ((_453 + _241) - _469)) * 0.984375)));
        }
        float _497 = -u_fragParams.u_earthCenter.w;
        float _504 = _241 - _215;
        float _505 = (fast::max((_497 * _310) + sqrt(fast::max((_192 * ((_310 * _310) - 1.0)) + _239, 0.0)), 0.0) - _215) / _504;
        float _507 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _504;
        float _514 = 0.015625 + ((fast::max(1.0 - (_505 / _507), 0.0) / (1.0 + _505)) * 0.96875);
        float _516 = (_311 + 1.0) * 3.5;
        float _517 = floor(_516);
        float _518 = _516 - _517;
        float _522 = _517 + 1.0;
        float4 _528 = scatteringTexture.sample(scatteringSampler, float3((_517 + _514) * 0.125, _492, _456));
        float _529 = 1.0 - _518;
        float4 _532 = scatteringTexture.sample(scatteringSampler, float3((_522 + _514) * 0.125, _492, _456));
        float4 _534 = (_528 * _529) + (_532 * _518);
        float3 _535 = _534.xyz;
        float3 _548;
        switch (0u)
        {
            default:
            {
                float _538 = _534.x;
                if (_538 == 0.0)
                {
                    _548 = float3(0.0);
                    break;
                }
                _548 = (((_535 * _534.w) / float3(_538)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _550 = fast::max(_313 - _286, 0.0);
        float _555 = fast::clamp(sqrt((_550 * (_550 + _325)) + _315), u_fragParams.u_earthCenter.w, _116);
        float _556 = _306 + _550;
        float _559 = (_309 + (_550 * _311)) / _555;
        float _560 = _555 * _555;
        float _563 = sqrt(fast::max(_560 - _192, 0.0));
        float _564 = _563 / _241;
        float _566 = 0.015625 + (_564 * 0.96875);
        float _569 = ((_556 * _556) - _560) + _192;
        float _602;
        if (_321)
        {
            float _592 = _555 - u_fragParams.u_earthCenter.w;
            _602 = 0.5 - (0.5 * (0.0078125 + (((_563 == _592) ? 0.0 : ((((-_556) - sqrt(fast::max(_569, 0.0))) - _592) / (_563 - _592))) * 0.984375)));
        }
        else
        {
            float _579 = _116 - _555;
            _602 = 0.5 + (0.5 * (0.0078125 + (((((-_556) + sqrt(fast::max(_569 + (_241 * _241), 0.0))) - _579) / ((_563 + _241) - _579)) * 0.984375)));
        }
        float _613 = (fast::max((_497 * _559) + sqrt(fast::max((_192 * ((_559 * _559) - 1.0)) + _239, 0.0)), 0.0) - _215) / _504;
        float _620 = 0.015625 + ((fast::max(1.0 - (_613 / _507), 0.0) / (1.0 + _613)) * 0.96875);
        float4 _628 = scatteringTexture.sample(scatteringSampler, float3((_517 + _620) * 0.125, _602, _566));
        float4 _631 = scatteringTexture.sample(scatteringSampler, float3((_522 + _620) * 0.125, _602, _566));
        float4 _633 = (_628 * _529) + (_631 * _518);
        float3 _634 = _633.xyz;
        float3 _647;
        switch (0u)
        {
            default:
            {
                float _637 = _633.x;
                if (_637 == 0.0)
                {
                    _647 = float3(0.0);
                    break;
                }
                _647 = (((_634 * _633.w) / float3(_637)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _754;
        if (_286 > 0.0)
        {
            float3 _753;
            switch (0u)
            {
                default:
                {
                    float _654 = fast::clamp(_556 / _555, -1.0, 1.0);
                    if (_321)
                    {
                        float _703 = -_654;
                        float _714 = _116 - _555;
                        float _727 = -_308;
                        float _738 = _116 - _307;
                        _753 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_555) * _703) + sqrt(fast::max((_560 * ((_703 * _703) - 1.0)) + _239, 0.0)), 0.0) - _714) / ((_563 + _241) - _714)) * 0.99609375), 0.0078125 + (_564 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_307) * _727) + sqrt(fast::max((_315 * ((_727 * _727) - 1.0)) + _239, 0.0)), 0.0) - _738) / ((_453 + _241) - _738)) * 0.99609375), 0.0078125 + (_454 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                    else
                    {
                        float _665 = _116 - _307;
                        float _688 = _116 - _555;
                        _753 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_307) * _308) + sqrt(fast::max(_318 + _239, 0.0)), 0.0) - _665) / ((_453 + _241) - _665)) * 0.99609375), 0.0078125 + (_454 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_555) * _654) + sqrt(fast::max((_560 * ((_654 * _654) - 1.0)) + _239, 0.0)), 0.0) - _688) / ((_563 + _241) - _688)) * 0.99609375), 0.0078125 + (_564 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                }
            }
            _754 = _753;
        }
        else
        {
            _754 = _450;
        }
        float3 _756 = _535 - (_754 * _634);
        float3 _758 = _548 - (_754 * _647);
        float _759 = _758.x;
        float _760 = _756.x;
        float3 _775;
        switch (0u)
        {
            default:
            {
                if (_760 == 0.0)
                {
                    _775 = float3(0.0);
                    break;
                }
                _775 = (((float4(_760, _756.yz, _759).xyz * _759) / float3(_760)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _779 = 1.0 + (_311 * _311);
        _793 = (((_173.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_269.xyz * smoothstep(_233 * (-0.004674999974668025970458984375), _233 * 0.004674999974668025970458984375, _213 - (-sqrt(fast::max(1.0 - (_233 * _233), 0.0))))) * fast::max(dot(_177, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_226.xyz * (1.0 + (dot(_177, _208) / _211))) * 0.5))) * _450) + (((_756 * (0.0596831031143665313720703125 * _779)) + ((_775 * smoothstep(0.0, 0.00999999977648258209228515625, _310)) * ((0.01627720706164836883544921875 * _779) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _311), 1.5)))) * mix(0.5, 1.0, fast::min(1.0, pow(_142, 6.0) * 6.0)));
    }
    else
    {
        _793 = float3(0.0);
    }
    float3 _1270;
    if (_196)
    {
        float3 _797 = _201 - u_fragParams.u_earthCenter.xyz;
        float _800 = length(_797);
        float _802 = dot(_797, u_fragParams.u_sunDirection.xyz) / _800;
        float _804 = _116 - u_fragParams.u_earthCenter.w;
        float4 _815 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_802 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_800 - u_fragParams.u_earthCenter.w) / _804) * 0.9375)));
        float _822 = u_fragParams.u_earthCenter.w / _800;
        float _828 = _116 * _116;
        float _830 = sqrt(_828 - _192);
        float _831 = _800 * _800;
        float _834 = sqrt(fast::max(_831 - _192, 0.0));
        float _845 = _116 - _800;
        float4 _858 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_800) * _802) + sqrt(fast::max((_831 * ((_802 * _802) - 1.0)) + _828, 0.0)), 0.0) - _845) / ((_834 + _830) - _845)) * 0.99609375), 0.0078125 + ((_834 / _830) * 0.984375)));
        float3 _876 = normalize(_797 - _186);
        float _877 = length(_186);
        float _878 = dot(_186, _876);
        float _885 = (-_878) - sqrt(((_878 * _878) - (_877 * _877)) + _828);
        bool _886 = _885 > 0.0;
        float3 _892;
        float _893;
        if (_886)
        {
            _892 = _186 + (_876 * _885);
            _893 = _878 + _885;
        }
        else
        {
            _892 = _186;
            _893 = _878;
        }
        float _912;
        float _894 = _886 ? _116 : _877;
        float _895 = _893 / _894;
        float _896 = dot(_892, u_fragParams.u_sunDirection.xyz);
        float _897 = _896 / _894;
        float _898 = dot(_876, u_fragParams.u_sunDirection.xyz);
        float _900 = length(_797 - _892);
        float _902 = _894 * _894;
        float _905 = _902 * ((_895 * _895) - 1.0);
        bool _908 = (_895 < 0.0) && ((_905 + _192) >= 0.0);
        float3 _1037;
        switch (0u)
        {
            default:
            {
                _912 = (2.0 * _894) * _895;
                float _917 = fast::clamp(sqrt((_900 * (_900 + _912)) + _902), u_fragParams.u_earthCenter.w, _116);
                float _920 = fast::clamp((_893 + _900) / _917, -1.0, 1.0);
                if (_908)
                {
                    float _978 = -_920;
                    float _979 = _917 * _917;
                    float _982 = sqrt(fast::max(_979 - _192, 0.0));
                    float _993 = _116 - _917;
                    float _1007 = -_895;
                    float _1010 = sqrt(fast::max(_902 - _192, 0.0));
                    float _1021 = _116 - _894;
                    _1037 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_917) * _978) + sqrt(fast::max((_979 * ((_978 * _978) - 1.0)) + _828, 0.0)), 0.0) - _993) / ((_982 + _830) - _993)) * 0.99609375), 0.0078125 + ((_982 / _830) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_894) * _1007) + sqrt(fast::max((_902 * ((_1007 * _1007) - 1.0)) + _828, 0.0)), 0.0) - _1021) / ((_1010 + _830) - _1021)) * 0.99609375), 0.0078125 + ((_1010 / _830) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _926 = sqrt(fast::max(_902 - _192, 0.0));
                    float _934 = _116 - _894;
                    float _948 = _917 * _917;
                    float _951 = sqrt(fast::max(_948 - _192, 0.0));
                    float _962 = _116 - _917;
                    _1037 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_894) * _895) + sqrt(fast::max(_905 + _828, 0.0)), 0.0) - _934) / ((_926 + _830) - _934)) * 0.99609375), 0.0078125 + ((_926 / _830) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_917) * _920) + sqrt(fast::max((_948 * ((_920 * _920) - 1.0)) + _828, 0.0)), 0.0) - _962) / ((_951 + _830) - _962)) * 0.99609375), 0.0078125 + ((_951 / _830) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _1040 = sqrt(fast::max(_902 - _192, 0.0));
        float _1043 = 0.015625 + ((_1040 / _830) * 0.96875);
        float _1046 = ((_893 * _893) - _902) + _192;
        float _1079;
        if (_908)
        {
            float _1069 = _894 - u_fragParams.u_earthCenter.w;
            _1079 = 0.5 - (0.5 * (0.0078125 + (((_1040 == _1069) ? 0.0 : ((((-_893) - sqrt(fast::max(_1046, 0.0))) - _1069) / (_1040 - _1069))) * 0.984375)));
        }
        else
        {
            float _1056 = _116 - _894;
            _1079 = 0.5 + (0.5 * (0.0078125 + (((((-_893) + sqrt(fast::max(_1046 + (_830 * _830), 0.0))) - _1056) / ((_1040 + _830) - _1056)) * 0.984375)));
        }
        float _1084 = -u_fragParams.u_earthCenter.w;
        float _1091 = _830 - _804;
        float _1092 = (fast::max((_1084 * _897) + sqrt(fast::max((_192 * ((_897 * _897) - 1.0)) + _828, 0.0)), 0.0) - _804) / _1091;
        float _1094 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1091;
        float _1101 = 0.015625 + ((fast::max(1.0 - (_1092 / _1094), 0.0) / (1.0 + _1092)) * 0.96875);
        float _1103 = (_898 + 1.0) * 3.5;
        float _1104 = floor(_1103);
        float _1105 = _1103 - _1104;
        float _1109 = _1104 + 1.0;
        float4 _1115 = scatteringTexture.sample(scatteringSampler, float3((_1104 + _1101) * 0.125, _1079, _1043));
        float _1116 = 1.0 - _1105;
        float4 _1119 = scatteringTexture.sample(scatteringSampler, float3((_1109 + _1101) * 0.125, _1079, _1043));
        float4 _1121 = (_1115 * _1116) + (_1119 * _1105);
        float3 _1122 = _1121.xyz;
        float3 _1135;
        switch (0u)
        {
            default:
            {
                float _1125 = _1121.x;
                if (_1125 == 0.0)
                {
                    _1135 = float3(0.0);
                    break;
                }
                _1135 = (((_1122 * _1121.w) / float3(_1125)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1136 = fast::max(_900, 0.0);
        float _1141 = fast::clamp(sqrt((_1136 * (_1136 + _912)) + _902), u_fragParams.u_earthCenter.w, _116);
        float _1142 = _893 + _1136;
        float _1145 = (_896 + (_1136 * _898)) / _1141;
        float _1146 = _1141 * _1141;
        float _1149 = sqrt(fast::max(_1146 - _192, 0.0));
        float _1152 = 0.015625 + ((_1149 / _830) * 0.96875);
        float _1155 = ((_1142 * _1142) - _1146) + _192;
        float _1188;
        if (_908)
        {
            float _1178 = _1141 - u_fragParams.u_earthCenter.w;
            _1188 = 0.5 - (0.5 * (0.0078125 + (((_1149 == _1178) ? 0.0 : ((((-_1142) - sqrt(fast::max(_1155, 0.0))) - _1178) / (_1149 - _1178))) * 0.984375)));
        }
        else
        {
            float _1165 = _116 - _1141;
            _1188 = 0.5 + (0.5 * (0.0078125 + (((((-_1142) + sqrt(fast::max(_1155 + (_830 * _830), 0.0))) - _1165) / ((_1149 + _830) - _1165)) * 0.984375)));
        }
        float _1199 = (fast::max((_1084 * _1145) + sqrt(fast::max((_192 * ((_1145 * _1145) - 1.0)) + _828, 0.0)), 0.0) - _804) / _1091;
        float _1206 = 0.015625 + ((fast::max(1.0 - (_1199 / _1094), 0.0) / (1.0 + _1199)) * 0.96875);
        float4 _1214 = scatteringTexture.sample(scatteringSampler, float3((_1104 + _1206) * 0.125, _1188, _1152));
        float4 _1217 = scatteringTexture.sample(scatteringSampler, float3((_1109 + _1206) * 0.125, _1188, _1152));
        float4 _1219 = (_1214 * _1116) + (_1217 * _1105);
        float3 _1220 = _1219.xyz;
        float3 _1233;
        switch (0u)
        {
            default:
            {
                float _1223 = _1219.x;
                if (_1223 == 0.0)
                {
                    _1233 = float3(0.0);
                    break;
                }
                _1233 = (((_1220 * _1219.w) / float3(_1223)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _1235 = _1122 - (_1037 * _1220);
        float3 _1237 = _1135 - (_1037 * _1233);
        float _1238 = _1237.x;
        float _1239 = _1235.x;
        float3 _1254;
        switch (0u)
        {
            default:
            {
                if (_1239 == 0.0)
                {
                    _1254 = float3(0.0);
                    break;
                }
                _1254 = (((float4(_1239, _1235.yz, _1238).xyz * _1238) / float3(_1239)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1258 = 1.0 + (_898 * _898);
        _1270 = (((_173.xyz * 0.3183098733425140380859375) * ((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * fast::max((_858.xyz * smoothstep(_822 * (-0.004674999974668025970458984375), _822 * 0.004674999974668025970458984375, _802 - (-sqrt(fast::max(1.0 - (_822 * _822), 0.0))))) * fast::max(dot(_177, u_fragParams.u_sunDirection.xyz), 0.0), float3(0.001000000047497451305389404296875))) + ((_815.xyz * (1.0 + (dot(_177, _797) / _800))) * 0.5))) * _1037) + ((_1235 * (0.0596831031143665313720703125 * _1258)) + ((_1254 * smoothstep(0.0, 0.00999999977648258209228515625, _897)) * ((0.01627720706164836883544921875 * _1258) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _898), 1.5))));
    }
    else
    {
        _1270 = float3(0.0);
    }
    float3 _1440;
    float3 _1441;
    switch (0u)
    {
        default:
        {
            float _1276 = length(_186);
            float _1279 = _116 * _116;
            float _1282 = _191 - sqrt((_189 - (_1276 * _1276)) + _1279);
            bool _1283 = _1282 > 0.0;
            float3 _1293;
            float _1294;
            if (_1283)
            {
                _1293 = _186 + (_129 * _1282);
                _1294 = _187 + _1282;
            }
            else
            {
                if (_1276 > _116)
                {
                    _1440 = float3(1.0);
                    _1441 = float3(0.0);
                    break;
                }
                _1293 = _186;
                _1294 = _187;
            }
            float _1295 = _1283 ? _116 : _1276;
            float _1296 = _1294 / _1295;
            float _1298 = dot(_1293, u_fragParams.u_sunDirection.xyz) / _1295;
            float _1299 = dot(_129, u_fragParams.u_sunDirection.xyz);
            float _1301 = _1295 * _1295;
            float _1304 = _1301 * ((_1296 * _1296) - 1.0);
            bool _1307 = (_1296 < 0.0) && ((_1304 + _192) >= 0.0);
            float _1309 = sqrt(_1279 - _192);
            float _1312 = sqrt(fast::max(_1301 - _192, 0.0));
            float _1320 = _116 - _1295;
            float _1323 = (_1312 + _1309) - _1320;
            float _1325 = _1312 / _1309;
            float4 _1333 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_1295) * _1296) + sqrt(fast::max(_1304 + _1279, 0.0)), 0.0) - _1320) / _1323) * 0.99609375), 0.0078125 + (_1325 * 0.984375)));
            float _1338 = 0.015625 + (_1325 * 0.96875);
            float _1341 = ((_1294 * _1294) - _1301) + _192;
            float _1371;
            if (_1307)
            {
                float _1361 = _1295 - u_fragParams.u_earthCenter.w;
                _1371 = 0.5 - (0.5 * (0.0078125 + (((_1312 == _1361) ? 0.0 : ((((-_1294) - sqrt(fast::max(_1341, 0.0))) - _1361) / (_1312 - _1361))) * 0.984375)));
            }
            else
            {
                _1371 = 0.5 + (0.5 * (0.0078125 + (((((-_1294) + sqrt(fast::max(_1341 + (_1309 * _1309), 0.0))) - _1320) / _1323) * 0.984375)));
            }
            float _1382 = _116 - u_fragParams.u_earthCenter.w;
            float _1384 = _1309 - _1382;
            float _1385 = (fast::max(((-u_fragParams.u_earthCenter.w) * _1298) + sqrt(fast::max((_192 * ((_1298 * _1298) - 1.0)) + _1279, 0.0)), 0.0) - _1382) / _1384;
            float _1394 = 0.015625 + ((fast::max(1.0 - (_1385 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1384)), 0.0) / (1.0 + _1385)) * 0.96875);
            float _1396 = (_1299 + 1.0) * 3.5;
            float _1397 = floor(_1396);
            float _1398 = _1396 - _1397;
            float4 _1408 = scatteringTexture.sample(scatteringSampler, float3((_1397 + _1394) * 0.125, _1371, _1338));
            float4 _1412 = scatteringTexture.sample(scatteringSampler, float3(((_1397 + 1.0) + _1394) * 0.125, _1371, _1338));
            float4 _1414 = (_1408 * (1.0 - _1398)) + (_1412 * _1398);
            float3 _1415 = _1414.xyz;
            float3 _1428;
            switch (0u)
            {
                default:
                {
                    float _1418 = _1414.x;
                    if (_1418 == 0.0)
                    {
                        _1428 = float3(0.0);
                        break;
                    }
                    _1428 = (((_1415 * _1414.w) / float3(_1418)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1430 = 1.0 + (_1299 * _1299);
            _1440 = select(_1333.xyz, float3(0.0), bool3(_1307));
            _1441 = (_1415 * (0.0596831031143665313720703125 * _1430)) + (_1428 * ((0.01627720706164836883544921875 * _1430) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1299), 1.5)));
            break;
        }
    }
    float3 _1449;
    if (dot(_129, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1449 = _1441 + (_1440 * float3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1449 = _1441;
    }
    float3 _1467 = pow(abs(float3(1.0) - exp(((-mix(mix(_1449, _1270, float3(float(_196))), _793, float3(float(_205) * fast::min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _142))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), float3(0.4545454680919647216796875));
    float4 _1469 = float4(_1467.x, _1467.y, _1467.z, _108.w);
    _1469.w = 1.0;
    out.out_var_SV_Target0 = _1469;
    out.out_var_SV_Target1 = _137;
    out.gl_FragDepth = _142;
    return out;
}

