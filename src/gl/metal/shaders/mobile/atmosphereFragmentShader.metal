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

fragment main0_out main0(main0_in in [[stage_in]], constant type_u_cameraPlaneParams& u_cameraPlaneParams [[buffer(0)]], constant type_u_fragParams& u_fragParams [[buffer(1)]], texture2d<float> transmittance_textureTexture [[texture(0)]], texture3d<float> scattering_textureTexture [[texture(1)]], texture2d<float> irradiance_textureTexture [[texture(2)]], texture2d<float> sceneColourTexture [[texture(3)]], texture2d<float> sceneDepthTexture [[texture(4)]], sampler transmittance_textureSampler [[sampler(0)]], sampler scattering_textureSampler [[sampler(1)]], sampler irradiance_textureSampler [[sampler(2)]], sampler sceneColourSampler [[sampler(3)]], sampler sceneDepthSampler [[sampler(4)]])
{
    main0_out out = {};
    float _111 = u_fragParams.u_earthCenter.w + 60000.0;
    float3 _124 = normalize(in.in_var_TEXCOORD1);
    float4 _128 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _129 = _128.x;
    float _134 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float4 _149 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0);
    float3 _152 = pow(abs(_149.xyz), float3(2.2000000476837158203125));
    float3 _153 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    bool _163 = _129 < 0.64999997615814208984375;
    float3 _642;
    if (_163)
    {
        float3 _168 = (u_fragParams.u_camera.xyz + (_124 * (((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _134) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _129 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _134))) * u_cameraPlaneParams.s_CameraFarPlane))) - u_fragParams.u_earthCenter.xyz;
        float3 _169 = normalize(_168);
        float _172 = length(_168);
        float _174 = dot(_168, u_fragParams.u_sunDirection.xyz) / _172;
        float _176 = _111 - u_fragParams.u_earthCenter.w;
        float4 _187 = irradiance_textureTexture.sample(irradiance_textureSampler, float2(0.0078125 + (((_174 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_172 - u_fragParams.u_earthCenter.w) / _176) * 0.9375)));
        float _194 = u_fragParams.u_earthCenter.w / _172;
        float _200 = _111 * _111;
        float _201 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        float _203 = sqrt(_200 - _201);
        float _204 = _172 * _172;
        float _207 = sqrt(fast::max(_204 - _201, 0.0));
        float _218 = _111 - _172;
        float4 _231 = transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_172) * _174) + sqrt(fast::max((_204 * ((_174 * _174) - 1.0)) + _200, 0.0)), 0.0) - _218) / ((_207 + _203) - _218)) * 0.99609375), 0.0078125 + ((_207 / _203) * 0.984375)));
        float3 _248 = normalize(_168 - _153);
        float _249 = length(_153);
        float _250 = dot(_153, _248);
        float _257 = (-_250) - sqrt(((_250 * _250) - (_249 * _249)) + _200);
        bool _258 = _257 > 0.0;
        float3 _264;
        float _265;
        if (_258)
        {
            _264 = _153 + (_248 * _257);
            _265 = _250 + _257;
        }
        else
        {
            _264 = _153;
            _265 = _250;
        }
        float _284;
        float _266 = _258 ? _111 : _249;
        float _267 = _265 / _266;
        float _268 = dot(_264, u_fragParams.u_sunDirection.xyz);
        float _269 = _268 / _266;
        float _270 = dot(_248, u_fragParams.u_sunDirection.xyz);
        float _272 = length(_168 - _264);
        float _274 = _266 * _266;
        float _277 = _274 * ((_267 * _267) - 1.0);
        bool _280 = (_267 < 0.0) && ((_277 + _201) >= 0.0);
        float3 _409;
        switch (0u)
        {
            default:
            {
                _284 = (2.0 * _266) * _267;
                float _289 = fast::clamp(sqrt((_272 * (_272 + _284)) + _274), u_fragParams.u_earthCenter.w, _111);
                float _292 = fast::clamp((_265 + _272) / _289, -1.0, 1.0);
                if (_280)
                {
                    float _350 = -_292;
                    float _351 = _289 * _289;
                    float _354 = sqrt(fast::max(_351 - _201, 0.0));
                    float _365 = _111 - _289;
                    float _379 = -_267;
                    float _382 = sqrt(fast::max(_274 - _201, 0.0));
                    float _393 = _111 - _266;
                    _409 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_289) * _350) + sqrt(fast::max((_351 * ((_350 * _350) - 1.0)) + _200, 0.0)), 0.0) - _365) / ((_354 + _203) - _365)) * 0.99609375), 0.0078125 + ((_354 / _203) * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_266) * _379) + sqrt(fast::max((_274 * ((_379 * _379) - 1.0)) + _200, 0.0)), 0.0) - _393) / ((_382 + _203) - _393)) * 0.99609375), 0.0078125 + ((_382 / _203) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _298 = sqrt(fast::max(_274 - _201, 0.0));
                    float _306 = _111 - _266;
                    float _320 = _289 * _289;
                    float _323 = sqrt(fast::max(_320 - _201, 0.0));
                    float _334 = _111 - _289;
                    _409 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_266) * _267) + sqrt(fast::max(_277 + _200, 0.0)), 0.0) - _306) / ((_298 + _203) - _306)) * 0.99609375), 0.0078125 + ((_298 / _203) * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_289) * _292) + sqrt(fast::max((_320 * ((_292 * _292) - 1.0)) + _200, 0.0)), 0.0) - _334) / ((_323 + _203) - _334)) * 0.99609375), 0.0078125 + ((_323 / _203) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _412 = sqrt(fast::max(_274 - _201, 0.0));
        float _415 = 0.015625 + ((_412 / _203) * 0.96875);
        float _418 = ((_265 * _265) - _274) + _201;
        float _451;
        if (_280)
        {
            float _441 = _266 - u_fragParams.u_earthCenter.w;
            _451 = 0.5 - (0.5 * (0.0078125 + (((_412 == _441) ? 0.0 : ((((-_265) - sqrt(fast::max(_418, 0.0))) - _441) / (_412 - _441))) * 0.984375)));
        }
        else
        {
            float _428 = _111 - _266;
            _451 = 0.5 + (0.5 * (0.0078125 + (((((-_265) + sqrt(fast::max(_418 + (_203 * _203), 0.0))) - _428) / ((_412 + _203) - _428)) * 0.984375)));
        }
        float _456 = -u_fragParams.u_earthCenter.w;
        float _463 = _203 - _176;
        float _464 = (fast::max((_456 * _269) + sqrt(fast::max((_201 * ((_269 * _269) - 1.0)) + _200, 0.0)), 0.0) - _176) / _463;
        float _466 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _463;
        float _473 = 0.015625 + ((fast::max(1.0 - (_464 / _466), 0.0) / (1.0 + _464)) * 0.96875);
        float _475 = (_270 + 1.0) * 3.5;
        float _476 = floor(_475);
        float _477 = _475 - _476;
        float _481 = _476 + 1.0;
        float4 _487 = scattering_textureTexture.sample(scattering_textureSampler, float3((_476 + _473) * 0.125, _451, _415));
        float _488 = 1.0 - _477;
        float4 _491 = scattering_textureTexture.sample(scattering_textureSampler, float3((_481 + _473) * 0.125, _451, _415));
        float4 _493 = (_487 * _488) + (_491 * _477);
        float3 _494 = _493.xyz;
        float3 _507;
        switch (0u)
        {
            default:
            {
                float _497 = _493.x;
                if (_497 == 0.0)
                {
                    _507 = float3(0.0);
                    break;
                }
                _507 = (((_494 * _493.w) / float3(_497)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _508 = fast::max(_272, 0.0);
        float _513 = fast::clamp(sqrt((_508 * (_508 + _284)) + _274), u_fragParams.u_earthCenter.w, _111);
        float _514 = _265 + _508;
        float _517 = (_268 + (_508 * _270)) / _513;
        float _518 = _513 * _513;
        float _521 = sqrt(fast::max(_518 - _201, 0.0));
        float _524 = 0.015625 + ((_521 / _203) * 0.96875);
        float _527 = ((_514 * _514) - _518) + _201;
        float _560;
        if (_280)
        {
            float _550 = _513 - u_fragParams.u_earthCenter.w;
            _560 = 0.5 - (0.5 * (0.0078125 + (((_521 == _550) ? 0.0 : ((((-_514) - sqrt(fast::max(_527, 0.0))) - _550) / (_521 - _550))) * 0.984375)));
        }
        else
        {
            float _537 = _111 - _513;
            _560 = 0.5 + (0.5 * (0.0078125 + (((((-_514) + sqrt(fast::max(_527 + (_203 * _203), 0.0))) - _537) / ((_521 + _203) - _537)) * 0.984375)));
        }
        float _571 = (fast::max((_456 * _517) + sqrt(fast::max((_201 * ((_517 * _517) - 1.0)) + _200, 0.0)), 0.0) - _176) / _463;
        float _578 = 0.015625 + ((fast::max(1.0 - (_571 / _466), 0.0) / (1.0 + _571)) * 0.96875);
        float4 _586 = scattering_textureTexture.sample(scattering_textureSampler, float3((_476 + _578) * 0.125, _560, _524));
        float4 _589 = scattering_textureTexture.sample(scattering_textureSampler, float3((_481 + _578) * 0.125, _560, _524));
        float4 _591 = (_586 * _488) + (_589 * _477);
        float3 _592 = _591.xyz;
        float3 _605;
        switch (0u)
        {
            default:
            {
                float _595 = _591.x;
                if (_595 == 0.0)
                {
                    _605 = float3(0.0);
                    break;
                }
                _605 = (((_592 * _591.w) / float3(_595)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _607 = _494 - (_409 * _592);
        float3 _609 = _507 - (_409 * _605);
        float _610 = _609.x;
        float _611 = _607.x;
        float3 _626;
        switch (0u)
        {
            default:
            {
                if (_611 == 0.0)
                {
                    _626 = float3(0.0);
                    break;
                }
                _626 = (((float4(_611, _607.yz, _610).xyz * _610) / float3(_611)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _630 = 1.0 + (_270 * _270);
        _642 = (((_152.xyz * 0.3183098733425140380859375) * (((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_231.xyz * smoothstep(_194 * (-0.004674999974668025970458984375), _194 * 0.004674999974668025970458984375, _174 - (-sqrt(fast::max(1.0 - (_194 * _194), 0.0)))))) * fast::max(dot(_169, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_187.xyz * (1.0 + (dot(_169, _168) / _172))) * 0.5))) * _409) + ((_607 * (0.0596831031143665313720703125 * _630)) + ((_626 * smoothstep(0.0, 0.00999999977648258209228515625, _269)) * ((0.01627720706164836883544921875 * _630) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _270), 1.5))));
    }
    else
    {
        _642 = float3(0.0);
    }
    float _644 = dot(_153, _124);
    float _646 = _644 * _644;
    float _648 = -_644;
    float _649 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _652 = _648 - sqrt(_649 - (dot(_153, _153) - _646));
    bool _653 = _652 > 0.0;
    float3 _1242;
    if (_653)
    {
        float3 _658 = (u_fragParams.u_camera.xyz + (_124 * _652)) - u_fragParams.u_earthCenter.xyz;
        float3 _659 = normalize(_658);
        float _662 = length(_658);
        float _664 = dot(_658, u_fragParams.u_sunDirection.xyz) / _662;
        float _666 = _111 - u_fragParams.u_earthCenter.w;
        float4 _677 = irradiance_textureTexture.sample(irradiance_textureSampler, float2(0.0078125 + (((_664 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_662 - u_fragParams.u_earthCenter.w) / _666) * 0.9375)));
        float _684 = u_fragParams.u_earthCenter.w / _662;
        float _690 = _111 * _111;
        float _692 = sqrt(_690 - _649);
        float _693 = _662 * _662;
        float _696 = sqrt(fast::max(_693 - _649, 0.0));
        float _707 = _111 - _662;
        float4 _720 = transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_662) * _664) + sqrt(fast::max((_693 * ((_664 * _664) - 1.0)) + _690, 0.0)), 0.0) - _707) / ((_696 + _692) - _707)) * 0.99609375), 0.0078125 + ((_696 / _692) * 0.984375)));
        float _737 = fast::max(0.0, fast::min(0.0, _652)) * smoothstep(0.0199999995529651641845703125, 0.039999999105930328369140625, dot(normalize(_153), u_fragParams.u_sunDirection.xyz));
        float3 _740 = normalize(_658 - _153);
        float _741 = length(_153);
        float _742 = dot(_153, _740);
        float _749 = (-_742) - sqrt(((_742 * _742) - (_741 * _741)) + _690);
        bool _750 = _749 > 0.0;
        float3 _756;
        float _757;
        if (_750)
        {
            _756 = _153 + (_740 * _749);
            _757 = _742 + _749;
        }
        else
        {
            _756 = _153;
            _757 = _742;
        }
        float _776;
        float _758 = _750 ? _111 : _741;
        float _759 = _757 / _758;
        float _760 = dot(_756, u_fragParams.u_sunDirection.xyz);
        float _761 = _760 / _758;
        float _762 = dot(_740, u_fragParams.u_sunDirection.xyz);
        float _764 = length(_658 - _756);
        float _766 = _758 * _758;
        float _769 = _766 * ((_759 * _759) - 1.0);
        bool _772 = (_759 < 0.0) && ((_769 + _649) >= 0.0);
        float3 _901;
        switch (0u)
        {
            default:
            {
                _776 = (2.0 * _758) * _759;
                float _781 = fast::clamp(sqrt((_764 * (_764 + _776)) + _766), u_fragParams.u_earthCenter.w, _111);
                float _784 = fast::clamp((_757 + _764) / _781, -1.0, 1.0);
                if (_772)
                {
                    float _842 = -_784;
                    float _843 = _781 * _781;
                    float _846 = sqrt(fast::max(_843 - _649, 0.0));
                    float _857 = _111 - _781;
                    float _871 = -_759;
                    float _874 = sqrt(fast::max(_766 - _649, 0.0));
                    float _885 = _111 - _758;
                    _901 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_781) * _842) + sqrt(fast::max((_843 * ((_842 * _842) - 1.0)) + _690, 0.0)), 0.0) - _857) / ((_846 + _692) - _857)) * 0.99609375), 0.0078125 + ((_846 / _692) * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_758) * _871) + sqrt(fast::max((_766 * ((_871 * _871) - 1.0)) + _690, 0.0)), 0.0) - _885) / ((_874 + _692) - _885)) * 0.99609375), 0.0078125 + ((_874 / _692) * 0.984375))).xyz, float3(1.0));
                    break;
                }
                else
                {
                    float _790 = sqrt(fast::max(_766 - _649, 0.0));
                    float _798 = _111 - _758;
                    float _812 = _781 * _781;
                    float _815 = sqrt(fast::max(_812 - _649, 0.0));
                    float _826 = _111 - _781;
                    _901 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_758) * _759) + sqrt(fast::max(_769 + _690, 0.0)), 0.0) - _798) / ((_790 + _692) - _798)) * 0.99609375), 0.0078125 + ((_790 / _692) * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_781) * _784) + sqrt(fast::max((_812 * ((_784 * _784) - 1.0)) + _690, 0.0)), 0.0) - _826) / ((_815 + _692) - _826)) * 0.99609375), 0.0078125 + ((_815 / _692) * 0.984375))).xyz, float3(1.0));
                    break;
                }
            }
        }
        float _904 = sqrt(fast::max(_766 - _649, 0.0));
        float _905 = _904 / _692;
        float _907 = 0.015625 + (_905 * 0.96875);
        float _910 = ((_757 * _757) - _766) + _649;
        float _943;
        if (_772)
        {
            float _933 = _758 - u_fragParams.u_earthCenter.w;
            _943 = 0.5 - (0.5 * (0.0078125 + (((_904 == _933) ? 0.0 : ((((-_757) - sqrt(fast::max(_910, 0.0))) - _933) / (_904 - _933))) * 0.984375)));
        }
        else
        {
            float _920 = _111 - _758;
            _943 = 0.5 + (0.5 * (0.0078125 + (((((-_757) + sqrt(fast::max(_910 + (_692 * _692), 0.0))) - _920) / ((_904 + _692) - _920)) * 0.984375)));
        }
        float _948 = -u_fragParams.u_earthCenter.w;
        float _955 = _692 - _666;
        float _956 = (fast::max((_948 * _761) + sqrt(fast::max((_649 * ((_761 * _761) - 1.0)) + _690, 0.0)), 0.0) - _666) / _955;
        float _958 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _955;
        float _965 = 0.015625 + ((fast::max(1.0 - (_956 / _958), 0.0) / (1.0 + _956)) * 0.96875);
        float _967 = (_762 + 1.0) * 3.5;
        float _968 = floor(_967);
        float _969 = _967 - _968;
        float _973 = _968 + 1.0;
        float4 _979 = scattering_textureTexture.sample(scattering_textureSampler, float3((_968 + _965) * 0.125, _943, _907));
        float _980 = 1.0 - _969;
        float4 _983 = scattering_textureTexture.sample(scattering_textureSampler, float3((_973 + _965) * 0.125, _943, _907));
        float4 _985 = (_979 * _980) + (_983 * _969);
        float3 _986 = _985.xyz;
        float3 _999;
        switch (0u)
        {
            default:
            {
                float _989 = _985.x;
                if (_989 == 0.0)
                {
                    _999 = float3(0.0);
                    break;
                }
                _999 = (((_986 * _985.w) / float3(_989)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1001 = fast::max(_764 - _737, 0.0);
        float _1006 = fast::clamp(sqrt((_1001 * (_1001 + _776)) + _766), u_fragParams.u_earthCenter.w, _111);
        float _1007 = _757 + _1001;
        float _1010 = (_760 + (_1001 * _762)) / _1006;
        float _1011 = _1006 * _1006;
        float _1014 = sqrt(fast::max(_1011 - _649, 0.0));
        float _1015 = _1014 / _692;
        float _1017 = 0.015625 + (_1015 * 0.96875);
        float _1020 = ((_1007 * _1007) - _1011) + _649;
        float _1053;
        if (_772)
        {
            float _1043 = _1006 - u_fragParams.u_earthCenter.w;
            _1053 = 0.5 - (0.5 * (0.0078125 + (((_1014 == _1043) ? 0.0 : ((((-_1007) - sqrt(fast::max(_1020, 0.0))) - _1043) / (_1014 - _1043))) * 0.984375)));
        }
        else
        {
            float _1030 = _111 - _1006;
            _1053 = 0.5 + (0.5 * (0.0078125 + (((((-_1007) + sqrt(fast::max(_1020 + (_692 * _692), 0.0))) - _1030) / ((_1014 + _692) - _1030)) * 0.984375)));
        }
        float _1064 = (fast::max((_948 * _1010) + sqrt(fast::max((_649 * ((_1010 * _1010) - 1.0)) + _690, 0.0)), 0.0) - _666) / _955;
        float _1071 = 0.015625 + ((fast::max(1.0 - (_1064 / _958), 0.0) / (1.0 + _1064)) * 0.96875);
        float4 _1079 = scattering_textureTexture.sample(scattering_textureSampler, float3((_968 + _1071) * 0.125, _1053, _1017));
        float4 _1082 = scattering_textureTexture.sample(scattering_textureSampler, float3((_973 + _1071) * 0.125, _1053, _1017));
        float4 _1084 = (_1079 * _980) + (_1082 * _969);
        float3 _1085 = _1084.xyz;
        float3 _1098;
        switch (0u)
        {
            default:
            {
                float _1088 = _1084.x;
                if (_1088 == 0.0)
                {
                    _1098 = float3(0.0);
                    break;
                }
                _1098 = (((_1085 * _1084.w) / float3(_1088)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float3 _1205;
        if (_737 > 0.0)
        {
            float3 _1204;
            switch (0u)
            {
                default:
                {
                    float _1105 = fast::clamp(_1007 / _1006, -1.0, 1.0);
                    if (_772)
                    {
                        float _1154 = -_1105;
                        float _1165 = _111 - _1006;
                        float _1178 = -_759;
                        float _1189 = _111 - _758;
                        _1204 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_1006) * _1154) + sqrt(fast::max((_1011 * ((_1154 * _1154) - 1.0)) + _690, 0.0)), 0.0) - _1165) / ((_1014 + _692) - _1165)) * 0.99609375), 0.0078125 + (_1015 * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_758) * _1178) + sqrt(fast::max((_766 * ((_1178 * _1178) - 1.0)) + _690, 0.0)), 0.0) - _1189) / ((_904 + _692) - _1189)) * 0.99609375), 0.0078125 + (_905 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                    else
                    {
                        float _1116 = _111 - _758;
                        float _1139 = _111 - _1006;
                        _1204 = fast::min(transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_758) * _759) + sqrt(fast::max(_769 + _690, 0.0)), 0.0) - _1116) / ((_904 + _692) - _1116)) * 0.99609375), 0.0078125 + (_905 * 0.984375))).xyz / transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_1006) * _1105) + sqrt(fast::max((_1011 * ((_1105 * _1105) - 1.0)) + _690, 0.0)), 0.0) - _1139) / ((_1014 + _692) - _1139)) * 0.99609375), 0.0078125 + (_1015 * 0.984375))).xyz, float3(1.0));
                        break;
                    }
                }
            }
            _1205 = _1204;
        }
        else
        {
            _1205 = _901;
        }
        float3 _1207 = _986 - (_1205 * _1085);
        float3 _1209 = _999 - (_1205 * _1098);
        float _1210 = _1209.x;
        float _1211 = _1207.x;
        float3 _1226;
        switch (0u)
        {
            default:
            {
                if (_1211 == 0.0)
                {
                    _1226 = float3(0.0);
                    break;
                }
                _1226 = (((float4(_1211, _1207.yz, _1210).xyz * _1210) / float3(_1211)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1230 = 1.0 + (_762 * _762);
        _1242 = (((_152.xyz * 0.3183098733425140380859375) * (((float3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_720.xyz * smoothstep(_684 * (-0.004674999974668025970458984375), _684 * 0.004674999974668025970458984375, _664 - (-sqrt(fast::max(1.0 - (_684 * _684), 0.0)))))) * fast::max(dot(_659, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_677.xyz * (1.0 + (dot(_659, _658) / _662))) * 0.5))) * _901) + ((_1207 * (0.0596831031143665313720703125 * _1230)) + ((_1226 * smoothstep(0.0, 0.00999999977648258209228515625, _761)) * ((0.01627720706164836883544921875 * _1230) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _762), 1.5))));
    }
    else
    {
        _1242 = float3(0.0);
    }
    float3 _1412;
    float3 _1413;
    switch (0u)
    {
        default:
        {
            float _1248 = length(_153);
            float _1251 = _111 * _111;
            float _1254 = _648 - sqrt((_646 - (_1248 * _1248)) + _1251);
            bool _1255 = _1254 > 0.0;
            float3 _1265;
            float _1266;
            if (_1255)
            {
                _1265 = _153 + (_124 * _1254);
                _1266 = _644 + _1254;
            }
            else
            {
                if (_1248 > _111)
                {
                    _1412 = float3(1.0);
                    _1413 = float3(0.0);
                    break;
                }
                _1265 = _153;
                _1266 = _644;
            }
            float _1267 = _1255 ? _111 : _1248;
            float _1268 = _1266 / _1267;
            float _1270 = dot(_1265, u_fragParams.u_sunDirection.xyz) / _1267;
            float _1271 = dot(_124, u_fragParams.u_sunDirection.xyz);
            float _1273 = _1267 * _1267;
            float _1276 = _1273 * ((_1268 * _1268) - 1.0);
            bool _1279 = (_1268 < 0.0) && ((_1276 + _649) >= 0.0);
            float _1281 = sqrt(_1251 - _649);
            float _1284 = sqrt(fast::max(_1273 - _649, 0.0));
            float _1292 = _111 - _1267;
            float _1295 = (_1284 + _1281) - _1292;
            float _1297 = _1284 / _1281;
            float4 _1305 = transmittance_textureTexture.sample(transmittance_textureSampler, float2(0.001953125 + (((fast::max(((-_1267) * _1268) + sqrt(fast::max(_1276 + _1251, 0.0)), 0.0) - _1292) / _1295) * 0.99609375), 0.0078125 + (_1297 * 0.984375)));
            float _1310 = 0.015625 + (_1297 * 0.96875);
            float _1313 = ((_1266 * _1266) - _1273) + _649;
            float _1343;
            if (_1279)
            {
                float _1333 = _1267 - u_fragParams.u_earthCenter.w;
                _1343 = 0.5 - (0.5 * (0.0078125 + (((_1284 == _1333) ? 0.0 : ((((-_1266) - sqrt(fast::max(_1313, 0.0))) - _1333) / (_1284 - _1333))) * 0.984375)));
            }
            else
            {
                _1343 = 0.5 + (0.5 * (0.0078125 + (((((-_1266) + sqrt(fast::max(_1313 + (_1281 * _1281), 0.0))) - _1292) / _1295) * 0.984375)));
            }
            float _1354 = _111 - u_fragParams.u_earthCenter.w;
            float _1356 = _1281 - _1354;
            float _1357 = (fast::max(((-u_fragParams.u_earthCenter.w) * _1270) + sqrt(fast::max((_649 * ((_1270 * _1270) - 1.0)) + _1251, 0.0)), 0.0) - _1354) / _1356;
            float _1366 = 0.015625 + ((fast::max(1.0 - (_1357 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1356)), 0.0) / (1.0 + _1357)) * 0.96875);
            float _1368 = (_1271 + 1.0) * 3.5;
            float _1369 = floor(_1368);
            float _1370 = _1368 - _1369;
            float4 _1380 = scattering_textureTexture.sample(scattering_textureSampler, float3((_1369 + _1366) * 0.125, _1343, _1310));
            float4 _1384 = scattering_textureTexture.sample(scattering_textureSampler, float3(((_1369 + 1.0) + _1366) * 0.125, _1343, _1310));
            float4 _1386 = (_1380 * (1.0 - _1370)) + (_1384 * _1370);
            float3 _1387 = _1386.xyz;
            float3 _1400;
            switch (0u)
            {
                default:
                {
                    float _1390 = _1386.x;
                    if (_1390 == 0.0)
                    {
                        _1400 = float3(0.0);
                        break;
                    }
                    _1400 = (((_1387 * _1386.w) / float3(_1390)) * 1.5) * float3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1402 = 1.0 + (_1271 * _1271);
            _1412 = select(_1305.xyz, float3(0.0), bool3(_1279));
            _1413 = (_1387 * (0.0596831031143665313720703125 * _1402)) + (_1400 * ((0.01627720706164836883544921875 * _1402) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1271), 1.5)));
            break;
        }
    }
    float3 _1421;
    if (dot(_124, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1421 = _1413 + (_1412 * float3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1421 = _1413;
    }
    float3 _1435 = pow(abs(float3(1.0) - exp(((-mix(mix(_1421, _1242, float3(float(_653))), _642, float3(float(_163)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), float3(0.4545454680919647216796875));
    float4 _1437 = float4(_1435.x, _1435.y, _1435.z, _103.w);
    _1437.w = 1.0;
    out.out_var_SV_Target = _1437;
    out.gl_FragDepth = _129;
    return out;
}

