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

constant float4 _103 = {};

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float3 in_var_TEXCOORD1 [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> transmittanceTexture [[texture(0)]], texture3d<float> scatteringTexture [[texture(1)]], texture2d<float> irradianceTexture [[texture(2)]], texture2d<float> sceneColourTexture [[texture(3)]], texture2d<float> sceneDepthTexture [[texture(4)]], sampler transmittanceSampler [[sampler(0)]], sampler scatteringSampler [[sampler(1)]], sampler irradianceSampler [[sampler(2)]], sampler sceneColourSampler [[sampler(3)]], sampler sceneDepthSampler [[sampler(4)]])
{
    main0_out out = {};
    float _111 = u_fragParams.u_earthCenter.w + 60000.0;
    float3 _124 = normalize(in.in_var_TEXCOORD1);
    float4 _128 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _129 = _128.x;
    float _134 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float4 _149 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0);
    float3 _152 = pow(abs(_149.xyz), float3(2.2000000476837158203125));
    float _158 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _134) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _129 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _134))) * u_cameraPlaneParams.s_CameraFarPlane;
    bool _161 = _129 < 0.64999997615814208984375;
    float3 _751;
    if (_161)
    {
        float3 _164 = (u_fragParams.u_camera.xyz + (_124 * _158)) - u_fragParams.u_earthCenter.xyz;
        float3 _165 = normalize(_164);
        float _168 = length(_164);
        float _170 = dot(_164, u_fragParams.u_sunDirection.xyz) / _168;
        float _172 = _111 - u_fragParams.u_earthCenter.w;
        float4 _183 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_170 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_168 - u_fragParams.u_earthCenter.w) / _172) * 0.9375)));
        float _190 = u_fragParams.u_earthCenter.w / _168;
        float _196 = _111 * _111;
        float _197 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        float _199 = sqrt(_196 - _197);
        float _200 = _168 * _168;
        float _203 = sqrt(fast::max(_200 - _197, 0.0));
        float _214 = _111 - _168;
        float4 _227 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_168) * _170) + sqrt(fast::max((_200 * ((_170 * _170) - 1.0)) + _196, 0.0)), 0.0) - _214) / ((_203 + _199) - _214)) * 0.99609375), 0.0078125 + ((_203 / _199) * 0.984375)));
        float _245 = mix(_158 * 0.64999997615814208984375, 0.0, pow(_129 * 1.53846156597137451171875, 8.0));
        float3 _246 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
        float3 _249 = normalize(_164 - _246);
        float _250 = length(_246);
        float _251 = dot(_246, _249);
        float _258 = (-_251) - sqrt(((_251 * _251) - (_250 * _250)) + _196);
        bool _259 = _258 > 0.0;
        float3 _265;
        float _266;
        if (_259)
        {
            _265 = _246 + (_249 * _258);
            _266 = _251 + _258;
        }
        else
        {
            _265 = _246;
            _266 = _251;
        }
        float _285;
        float _267 = _259 ? _111 : _250;
        float _268 = _266 / _267;
        float _269 = dot(_265, u_fragParams.u_sunDirection.xyz);
        float _270 = _269 / _267;
        float _271 = dot(_249, u_fragParams.u_sunDirection.xyz);
        float _273 = length(_164 - _265);
        float _275 = _267 * _267;
        float _278 = _275 * ((_268 * _268) - 1.0);
        bool _281 = (_268 < 0.0) && ((_278 + _197) >= 0.0);
        float3 _410;
        switch (0u)
        {
            default:
            {
                _285 = (2.0 * _267) * _268;
                float _290 = fast::clamp(sqrt((_273 * (_273 + _285)) + _275), u_fragParams.u_earthCenter.w, _111);
                float _293 = fast::clamp((_266 + _273) / _290, -1.0, 1.0);
                if (_281)
                {
                    float _351 = -_293;
                    float _352 = _290 * _290;
                    float _355 = sqrt(fast::max(_352 - _197, 0.0));
                    float _366 = _111 - _290;
                    float _380 = -_268;
                    float _383 = sqrt(fast::max(_275 - _197, 0.0));
                    float _394 = _111 - _267;
                    _410 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_290) * _351) + sqrt(fast::max((_352 * ((_351 * _351) - 1.0)) + _196, 0.0)), 0.0) - _366) / ((_355 + _199) - _366)) * 0.99609375), 0.0078125 + ((_355 / _199) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_267) * _380) + sqrt(fast::max((_275 * ((_380 * _380) - 1.0)) + _196, 0.0)), 0.0) - _394) / ((_383 + _199) - _394)) * 0.99609375), 0.0078125 + ((_383 / _199) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _299 = sqrt(fast::max(_275 - _197, 0.0));
                    float _307 = _111 - _267;
                    float _321 = _290 * _290;
                    float _324 = sqrt(fast::max(_321 - _197, 0.0));
                    float _335 = _111 - _290;
                    _410 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_267) * _268) + sqrt(fast::max(_278 + _196, 0.0)), 0.0) - _307) / ((_299 + _199) - _307)) * 0.99609375), 0.0078125 + ((_299 / _199) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_290) * _293) + sqrt(fast::max((_321 * ((_293 * _293) - 1.0)) + _196, 0.0)), 0.0) - _335) / ((_324 + _199) - _335)) * 0.99609375), 0.0078125 + ((_324 / _199) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _413 = sqrt(fast::max(_275 - _197, 0.0));
        float _414 = _413 / _199;
        float _416 = 0.015625 + (_414 * 0.96875);
        float _419 = ((_266 * _266) - _275) + _197;
        float _452;
        if (_281)
        {
            float _442 = _267 - u_fragParams.u_earthCenter.w;
            _452 = 0.5 - (0.5 * (0.0078125 + (((_413 == _442) ? 0.0 : ((((-_266) - sqrt(fast::max(_419, 0.0))) - _442) / (_413 - _442))) * 0.984375)));
        }
        else
        {
            float _429 = _111 - _267;
            _452 = 0.5 + (0.5 * (0.0078125 + (((((-_266) + sqrt(fast::max(_419 + (_199 * _199), 0.0))) - _429) / ((_413 + _199) - _429)) * 0.984375)));
        }
        float _457 = -u_fragParams.u_earthCenter.w;
        float _464 = _199 - _172;
        float _465 = (fast::max((_457 * _270) + sqrt(fast::max((_197 * ((_270 * _270) - 1.0)) + _196, 0.0)), 0.0) - _172) / _464;
        float _467 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _464;
        float _474 = 0.015625 + ((fast::max(1.0 - (_465 / _467), 0.0) / (1.0 + _465)) * 0.96875);
        float _476 = (_271 + 1.0) * 3.5;
        float _477 = floor(_476);
        float _478 = _476 - _477;
        float _482 = _477 + 1.0;
        float4 _488 = scatteringTexture.sample(scatteringSampler, float3((_477 + _474) * 0.125, _452, _416));
        float _489 = 1.0 - _478;
        float4 _492 = scatteringTexture.sample(scatteringSampler, float3((_482 + _474) * 0.125, _452, _416));
        float4 _494 = (_488 * _489) + (_492 * _478);
        float3 _495 = _494.xyz;
        float3 _508;
        switch (0u)
        {
            default:
            {
                float _498 = _494.x;
                if (_498 == 0.0)
                {
                    _508 = float3(0.0);
                    break;
                }
                _508 = (((_495 * _494.w) / float3(_498)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _510 = fast::max(_273 - _245, 0.0);
        float _515 = fast::clamp(sqrt((_510 * (_510 + _285)) + _275), u_fragParams.u_earthCenter.w, _111);
        float _516 = _266 + _510;
        float _519 = (_269 + (_510 * _271)) / _515;
        float _520 = _515 * _515;
        float _523 = sqrt(fast::max(_520 - _197, 0.0));
        float _524 = _523 / _199;
        float _526 = 0.015625 + (_524 * 0.96875);
        float _529 = ((_516 * _516) - _520) + _197;
        float _562;
        if (_281)
        {
            float _552 = _515 - u_fragParams.u_earthCenter.w;
            _562 = 0.5 - (0.5 * (0.0078125 + (((_523 == _552) ? 0.0 : ((((-_516) - sqrt(fast::max(_529, 0.0))) - _552) / (_523 - _552))) * 0.984375)));
        }
        else
        {
            float _539 = _111 - _515;
            _562 = 0.5 + (0.5 * (0.0078125 + (((((-_516) + sqrt(fast::max(_529 + (_199 * _199), 0.0))) - _539) / ((_523 + _199) - _539)) * 0.984375)));
        }
        float _573 = (fast::max((_457 * _519) + sqrt(fast::max((_197 * ((_519 * _519) - 1.0)) + _196, 0.0)), 0.0) - _172) / _464;
        float _580 = 0.015625 + ((fast::max(1.0 - (_573 / _467), 0.0) / (1.0 + _573)) * 0.96875);
        float4 _588 = scatteringTexture.sample(scatteringSampler, float3((_477 + _580) * 0.125, _562, _526));
        float4 _591 = scatteringTexture.sample(scatteringSampler, float3((_482 + _580) * 0.125, _562, _526));
        float4 _593 = (_588 * _489) + (_591 * _478);
        float3 _594 = _593.xyz;
        float3 _607;
        switch (0u)
        {
            default:
            {
                float _597 = _593.x;
                if (_597 == 0.0)
                {
                    _607 = float3(0.0);
                    break;
                }
                _607 = (((_594 * _593.w) / float3(_597)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _714;
        if (_245 > 0.0)
        {
            float3 _713;
            switch (0u)
            {
                default:
                {
                    float _614 = fast::clamp(_516 / _515, -1.0, 1.0);
                    if (_281)
                    {
                        float _663 = -_614;
                        float _674 = _111 - _515;
                        float _687 = -_268;
                        float _698 = _111 - _267;
                        _713 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_515) * _663) + sqrt(fast::max((_520 * ((_663 * _663) - 1.0)) + _196, 0.0)), 0.0) - _674) / ((_523 + _199) - _674)) * 0.99609375), 0.0078125 + (_524 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_267) * _687) + sqrt(fast::max((_275 * ((_687 * _687) - 1.0)) + _196, 0.0)), 0.0) - _698) / ((_413 + _199) - _698)) * 0.99609375), 0.0078125 + (_414 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                    else
                    {
                        float _625 = _111 - _267;
                        float _648 = _111 - _515;
                        _713 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_267) * _268) + sqrt(fast::max(_278 + _196, 0.0)), 0.0) - _625) / ((_413 + _199) - _625)) * 0.99609375), 0.0078125 + (_414 * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_515) * _614) + sqrt(fast::max((_520 * ((_614 * _614) - 1.0)) + _196, 0.0)), 0.0) - _648) / ((_523 + _199) - _648)) * 0.99609375), 0.0078125 + (_524 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                }
            }
            _714 = _713;
        }
        else
        {
            _714 = _410;
        }
        float3 _716 = _495 - (_714 * _594);
        float3 _718 = _508 - (_714 * _607);
        float _719 = _718.x;
        float _720 = _716.x;
        float3 _735;
        switch (0u)
        {
            default:
            {
                if (_720 == 0.0)
                {
                    _735 = float3(0.0);
                    break;
                }
                _735 = (((float4(_720, _716.yz, _719).xyz * _719) / float3(_720)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _739 = 1.0 + (_271 * _271);
        _751 = (((_152.xyz * 0.3183098733425140380859375) * (((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_227.xyz * smoothstep(_190 * (-0.004674999974668025970458984375), _190 * 0.004674999974668025970458984375, _170 - (-sqrt(fast::max(1.0 - (_190 * _190), 0.0)))))) * fast::max(dot(_165, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_183.xyz * (1.0 + (dot(_165, _164) / _168))) * 0.5))) * _410) + ((_716 * (0.0596831031143665313720703125 * _739)) + ((_735 * smoothstep(0.0, 0.00999999977648258209228515625, _270)) * ((0.01627720706164836883544921875 * _739) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _271), 1.5))));
    }
    else
    {
        _751 = float3(0.0);
    }
    float3 _753 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    float _754 = dot(_753, _124);
    float _756 = _754 * _754;
    float _758 = -_754;
    float _759 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _762 = _758 - sqrt(_759 - (dot(_753, _753) - _756));
    bool _763 = _762 > 0.0;
    float3 _1241;
    if (_763)
    {
        float3 _768 = (u_fragParams.u_camera.xyz + (_124 * _762)) - u_fragParams.u_earthCenter.xyz;
        float3 _769 = normalize(_768);
        float _772 = length(_768);
        float _774 = dot(_768, u_fragParams.u_sunDirection.xyz) / _772;
        float _776 = _111 - u_fragParams.u_earthCenter.w;
        float4 _787 = irradianceTexture.sample(irradianceSampler, float2(0.0078125 + (((_774 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_772 - u_fragParams.u_earthCenter.w) / _776) * 0.9375)));
        float _794 = u_fragParams.u_earthCenter.w / _772;
        float _800 = _111 * _111;
        float _802 = sqrt(_800 - _759);
        float _803 = _772 * _772;
        float _806 = sqrt(fast::max(_803 - _759, 0.0));
        float _817 = _111 - _772;
        float4 _830 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_772) * _774) + sqrt(fast::max((_803 * ((_774 * _774) - 1.0)) + _800, 0.0)), 0.0) - _817) / ((_806 + _802) - _817)) * 0.99609375), 0.0078125 + ((_806 / _802) * 0.984375)));
        float3 _847 = normalize(_768 - _753);
        float _848 = length(_753);
        float _849 = dot(_753, _847);
        float _856 = (-_849) - sqrt(((_849 * _849) - (_848 * _848)) + _800);
        bool _857 = _856 > 0.0;
        float3 _863;
        float _864;
        if (_857)
        {
            _863 = _753 + (_847 * _856);
            _864 = _849 + _856;
        }
        else
        {
            _863 = _753;
            _864 = _849;
        }
        float _883;
        float _865 = _857 ? _111 : _848;
        float _866 = _864 / _865;
        float _867 = dot(_863, u_fragParams.u_sunDirection.xyz);
        float _868 = _867 / _865;
        float _869 = dot(_847, u_fragParams.u_sunDirection.xyz);
        float _871 = length(_768 - _863);
        float _873 = _865 * _865;
        float _876 = _873 * ((_866 * _866) - 1.0);
        bool _879 = (_866 < 0.0) && ((_876 + _759) >= 0.0);
        float3 _1008;
        switch (0u)
        {
            default:
            {
                _883 = (2.0 * _865) * _866;
                float _888 = fast::clamp(sqrt((_871 * (_871 + _883)) + _873), u_fragParams.u_earthCenter.w, _111);
                float _891 = fast::clamp((_864 + _871) / _888, -1.0, 1.0);
                if (_879)
                {
                    float _949 = -_891;
                    float _950 = _888 * _888;
                    float _953 = sqrt(fast::max(_950 - _759, 0.0));
                    float _964 = _111 - _888;
                    float _978 = -_866;
                    float _981 = sqrt(fast::max(_873 - _759, 0.0));
                    float _992 = _111 - _865;
                    _1008 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_888) * _949) + sqrt(fast::max((_950 * ((_949 * _949) - 1.0)) + _800, 0.0)), 0.0) - _964) / ((_953 + _802) - _964)) * 0.99609375), 0.0078125 + ((_953 / _802) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_865) * _978) + sqrt(fast::max((_873 * ((_978 * _978) - 1.0)) + _800, 0.0)), 0.0) - _992) / ((_981 + _802) - _992)) * 0.99609375), 0.0078125 + ((_981 / _802) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _897 = sqrt(fast::max(_873 - _759, 0.0));
                    float _905 = _111 - _865;
                    float _919 = _888 * _888;
                    float _922 = sqrt(fast::max(_919 - _759, 0.0));
                    float _933 = _111 - _888;
                    _1008 = fast::min(transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_865) * _866) + sqrt(fast::max(_876 + _800, 0.0)), 0.0) - _905) / ((_897 + _802) - _905)) * 0.99609375), 0.0078125 + ((_897 / _802) * 0.984375))).xyz / transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_888) * _891) + sqrt(fast::max((_919 * ((_891 * _891) - 1.0)) + _800, 0.0)), 0.0) - _933) / ((_922 + _802) - _933)) * 0.99609375), 0.0078125 + ((_922 / _802) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _1011 = sqrt(fast::max(_873 - _759, 0.0));
        float _1014 = 0.015625 + ((_1011 / _802) * 0.96875);
        float _1017 = ((_864 * _864) - _873) + _759;
        float _1050;
        if (_879)
        {
            float _1040 = _865 - u_fragParams.u_earthCenter.w;
            _1050 = 0.5 - (0.5 * (0.0078125 + (((_1011 == _1040) ? 0.0 : ((((-_864) - sqrt(fast::max(_1017, 0.0))) - _1040) / (_1011 - _1040))) * 0.984375)));
        }
        else
        {
            float _1027 = _111 - _865;
            _1050 = 0.5 + (0.5 * (0.0078125 + (((((-_864) + sqrt(fast::max(_1017 + (_802 * _802), 0.0))) - _1027) / ((_1011 + _802) - _1027)) * 0.984375)));
        }
        float _1055 = -u_fragParams.u_earthCenter.w;
        float _1062 = _802 - _776;
        float _1063 = (fast::max((_1055 * _868) + sqrt(fast::max((_759 * ((_868 * _868) - 1.0)) + _800, 0.0)), 0.0) - _776) / _1062;
        float _1065 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1062;
        float _1072 = 0.015625 + ((fast::max(1.0 - (_1063 / _1065), 0.0) / (1.0 + _1063)) * 0.96875);
        float _1074 = (_869 + 1.0) * 3.5;
        float _1075 = floor(_1074);
        float _1076 = _1074 - _1075;
        float _1080 = _1075 + 1.0;
        float4 _1086 = scatteringTexture.sample(scatteringSampler, float3((_1075 + _1072) * 0.125, _1050, _1014));
        float _1087 = 1.0 - _1076;
        float4 _1090 = scatteringTexture.sample(scatteringSampler, float3((_1080 + _1072) * 0.125, _1050, _1014));
        float4 _1092 = (_1086 * _1087) + (_1090 * _1076);
        float3 _1093 = _1092.xyz;
        float3 _1106;
        switch (0u)
        {
            default:
            {
                float _1096 = _1092.x;
                if (_1096 == 0.0)
                {
                    _1106 = float3(0.0);
                    break;
                }
                _1106 = (((_1093 * _1092.w) / float3(_1096)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1107 = fast::max(_871, 0.0);
        float _1112 = fast::clamp(sqrt((_1107 * (_1107 + _883)) + _873), u_fragParams.u_earthCenter.w, _111);
        float _1113 = _864 + _1107;
        float _1116 = (_867 + (_1107 * _869)) / _1112;
        float _1117 = _1112 * _1112;
        float _1120 = sqrt(fast::max(_1117 - _759, 0.0));
        float _1123 = 0.015625 + ((_1120 / _802) * 0.96875);
        float _1126 = ((_1113 * _1113) - _1117) + _759;
        float _1159;
        if (_879)
        {
            float _1149 = _1112 - u_fragParams.u_earthCenter.w;
            _1159 = 0.5 - (0.5 * (0.0078125 + (((_1120 == _1149) ? 0.0 : ((((-_1113) - sqrt(fast::max(_1126, 0.0))) - _1149) / (_1120 - _1149))) * 0.984375)));
        }
        else
        {
            float _1136 = _111 - _1112;
            _1159 = 0.5 + (0.5 * (0.0078125 + (((((-_1113) + sqrt(fast::max(_1126 + (_802 * _802), 0.0))) - _1136) / ((_1120 + _802) - _1136)) * 0.984375)));
        }
        float _1170 = (fast::max((_1055 * _1116) + sqrt(fast::max((_759 * ((_1116 * _1116) - 1.0)) + _800, 0.0)), 0.0) - _776) / _1062;
        float _1177 = 0.015625 + ((fast::max(1.0 - (_1170 / _1065), 0.0) / (1.0 + _1170)) * 0.96875);
        float4 _1185 = scatteringTexture.sample(scatteringSampler, float3((_1075 + _1177) * 0.125, _1159, _1123));
        float4 _1188 = scatteringTexture.sample(scatteringSampler, float3((_1080 + _1177) * 0.125, _1159, _1123));
        float4 _1190 = (_1185 * _1087) + (_1188 * _1076);
        float3 _1191 = _1190.xyz;
        float3 _1204;
        switch (0u)
        {
            default:
            {
                float _1194 = _1190.x;
                if (_1194 == 0.0)
                {
                    _1204 = float3(0.0);
                    break;
                }
                _1204 = (((_1191 * _1190.w) / float3(_1194)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _1206 = _1093 - (_1008 * _1191);
        float3 _1208 = _1106 - (_1008 * _1204);
        float _1209 = _1208.x;
        float _1210 = _1206.x;
        float3 _1225;
        switch (0u)
        {
            default:
            {
                if (_1210 == 0.0)
                {
                    _1225 = float3(0.0);
                    break;
                }
                _1225 = (((float4(_1210, _1206.yz, _1209).xyz * _1209) / float3(_1210)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1229 = 1.0 + (_869 * _869);
        _1241 = (((_152.xyz * 0.3183098733425140380859375) * (((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_830.xyz * smoothstep(_794 * (-0.004674999974668025970458984375), _794 * 0.004674999974668025970458984375, _774 - (-sqrt(fast::max(1.0 - (_794 * _794), 0.0)))))) * fast::max(dot(_769, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_787.xyz * (1.0 + (dot(_769, _768) / _772))) * 0.5))) * _1008) + ((_1206 * (0.0596831031143665313720703125 * _1229)) + ((_1225 * smoothstep(0.0, 0.00999999977648258209228515625, _868)) * ((0.01627720706164836883544921875 * _1229) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _869), 1.5))));
    }
    else
    {
        _1241 = float3(0.0);
    }
    float3 _1411;
    float3 _1412;
    switch (0u)
    {
        default:
        {
            float _1247 = length(_753);
            float _1250 = _111 * _111;
            float _1253 = _758 - sqrt((_756 - (_1247 * _1247)) + _1250);
            bool _1254 = _1253 > 0.0;
            float3 _1264;
            float _1265;
            if (_1254)
            {
                _1264 = _753 + (_124 * _1253);
                _1265 = _754 + _1253;
            }
            else
            {
                if (_1247 > _111)
                {
                    _1411 = float3(1.0);
                    _1412 = float3(0.0);
                    break;
                }
                _1264 = _753;
                _1265 = _754;
            }
            float _1266 = _1254 ? _111 : _1247;
            float _1267 = _1265 / _1266;
            float _1269 = dot(_1264, u_fragParams.u_sunDirection.xyz) / _1266;
            float _1270 = dot(_124, u_fragParams.u_sunDirection.xyz);
            float _1272 = _1266 * _1266;
            float _1275 = _1272 * ((_1267 * _1267) - 1.0);
            bool _1278 = (_1267 < 0.0) && ((_1275 + _759) >= 0.0);
            float _1280 = sqrt(_1250 - _759);
            float _1283 = sqrt(fast::max(_1272 - _759, 0.0));
            float _1291 = _111 - _1266;
            float _1294 = (_1283 + _1280) - _1291;
            float _1296 = _1283 / _1280;
            float4 _1304 = transmittanceTexture.sample(transmittanceSampler, float2(0.001953125 + (((fast::max(((-_1266) * _1267) + sqrt(fast::max(_1275 + _1250, 0.0)), 0.0) - _1291) / _1294) * 0.99609375), 0.0078125 + (_1296 * 0.984375)));
            float _1309 = 0.015625 + (_1296 * 0.96875);
            float _1312 = ((_1265 * _1265) - _1272) + _759;
            float _1342;
            if (_1278)
            {
                float _1332 = _1266 - u_fragParams.u_earthCenter.w;
                _1342 = 0.5 - (0.5 * (0.0078125 + (((_1283 == _1332) ? 0.0 : ((((-_1265) - sqrt(fast::max(_1312, 0.0))) - _1332) / (_1283 - _1332))) * 0.984375)));
            }
            else
            {
                _1342 = 0.5 + (0.5 * (0.0078125 + (((((-_1265) + sqrt(fast::max(_1312 + (_1280 * _1280), 0.0))) - _1291) / _1294) * 0.984375)));
            }
            float _1353 = _111 - u_fragParams.u_earthCenter.w;
            float _1355 = _1280 - _1353;
            float _1356 = (fast::max(((-u_fragParams.u_earthCenter.w) * _1269) + sqrt(fast::max((_759 * ((_1269 * _1269) - 1.0)) + _1250, 0.0)), 0.0) - _1353) / _1355;
            float _1365 = 0.015625 + ((fast::max(1.0 - (_1356 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1355)), 0.0) / (1.0 + _1356)) * 0.96875);
            float _1367 = (_1270 + 1.0) * 3.5;
            float _1368 = floor(_1367);
            float _1369 = _1367 - _1368;
            float4 _1379 = scatteringTexture.sample(scatteringSampler, float3((_1368 + _1365) * 0.125, _1342, _1309));
            float4 _1383 = scatteringTexture.sample(scatteringSampler, float3(((_1368 + 1.0) + _1365) * 0.125, _1342, _1309));
            float4 _1385 = (_1379 * (1.0 - _1369)) + (_1383 * _1369);
            float3 _1386 = _1385.xyz;
            float3 _1399;
            switch (0u)
            {
                default:
                {
                    float _1389 = _1385.x;
                    if (_1389 == 0.0)
                    {
                        _1399 = float3(0.0);
                        break;
                    }
                    _1399 = (((_1386 * _1385.w) / float3(_1389)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1401 = 1.0 + (_1270 * _1270);
            _1411 = select(_1304.xyz, float3(0.0), bool3(_1278));
            _1412 = (_1386 * (0.0596831031143665313720703125 * _1401)) + (_1399 * ((0.01627720706164836883544921875 * _1401) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1270), 1.5)));
            break;
        }
    }
    float3 _1420;
    if (dot(_124, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1420 = _1412 + (_1411 * float3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1420 = _1412;
    }
    float3 _1434 = pow(abs(float3(1.0) - exp(((-mix(mix(_1420, _1241, float3(float(_763))), _751, float3(float(_161)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), float3(0.4545454680919647216796875));
    float4 _1436 = float4(_1434.x, _1434.y, _1434.z, _103.w);
    _1436.w = 1.0;
    out.out_var_SV_Target = _1436;
    out.gl_FragDepth = _129;
    return out;
}

