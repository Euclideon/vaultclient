#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_cameraPlaneParams
{
    highp float s_CameraNearPlane;
    highp float s_CameraFarPlane;
    highp float u_clipZNear;
    highp float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_fragParams
{
    highp vec4 u_camera;
    highp vec4 u_whitePoint;
    highp vec4 u_earthCenter;
    highp vec4 u_sunDirection;
    highp vec4 u_sunSize;
    layout(row_major) highp mat4 u_inverseProjection;
} u_fragParams;

uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform highp sampler2D SPIRV_Cross_CombinedirradianceTextureirradianceSampler;
uniform highp sampler2D SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler;
uniform highp sampler3D SPIRV_Cross_CombinedscatteringTexturescatteringSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec3 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

vec4 _103;

void main()
{
    highp float _111 = u_fragParams.u_earthCenter.w + 60000.0;
    highp vec3 _124 = normalize(varying_TEXCOORD1);
    highp vec4 _128 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0);
    highp float _129 = _128.x;
    highp float _134 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp vec4 _149 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0);
    highp vec3 _152 = pow(abs(_149.xyz), vec3(2.2000000476837158203125));
    highp vec3 _153 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    bool _163 = _129 < 0.64999997615814208984375;
    highp vec3 _642;
    if (_163)
    {
        highp vec3 _168 = (u_fragParams.u_camera.xyz + (_124 * (((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _134) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _129 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _134))) * u_cameraPlaneParams.s_CameraFarPlane))) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _169 = normalize(_168);
        highp float _172 = length(_168);
        highp float _174 = dot(_168, u_fragParams.u_sunDirection.xyz) / _172;
        highp float _176 = _111 - u_fragParams.u_earthCenter.w;
        highp vec4 _187 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_174 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_172 - u_fragParams.u_earthCenter.w) / _176) * 0.9375)));
        highp float _194 = u_fragParams.u_earthCenter.w / _172;
        highp float _200 = _111 * _111;
        highp float _201 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        highp float _203 = sqrt(_200 - _201);
        highp float _204 = _172 * _172;
        highp float _207 = sqrt(max(_204 - _201, 0.0));
        highp float _218 = _111 - _172;
        highp vec4 _231 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_172) * _174) + sqrt(max((_204 * ((_174 * _174) - 1.0)) + _200, 0.0)), 0.0) - _218) / ((_207 + _203) - _218)) * 0.99609375), 0.0078125 + ((_207 / _203) * 0.984375)));
        highp vec3 _248 = normalize(_168 - _153);
        highp float _249 = length(_153);
        highp float _250 = dot(_153, _248);
        highp float _257 = (-_250) - sqrt(((_250 * _250) - (_249 * _249)) + _200);
        bool _258 = _257 > 0.0;
        highp vec3 _264;
        highp float _265;
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
        highp float _284;
        highp float _266 = _258 ? _111 : _249;
        highp float _267 = _265 / _266;
        highp float _268 = dot(_264, u_fragParams.u_sunDirection.xyz);
        highp float _269 = _268 / _266;
        highp float _270 = dot(_248, u_fragParams.u_sunDirection.xyz);
        highp float _272 = length(_168 - _264);
        highp float _274 = _266 * _266;
        highp float _277 = _274 * ((_267 * _267) - 1.0);
        bool _280 = (_267 < 0.0) && ((_277 + _201) >= 0.0);
        highp vec3 _409;
        switch (0u)
        {
            default:
            {
                _284 = (2.0 * _266) * _267;
                highp float _289 = clamp(sqrt((_272 * (_272 + _284)) + _274), u_fragParams.u_earthCenter.w, _111);
                highp float _292 = clamp((_265 + _272) / _289, -1.0, 1.0);
                if (_280)
                {
                    highp float _350 = -_292;
                    highp float _351 = _289 * _289;
                    highp float _354 = sqrt(max(_351 - _201, 0.0));
                    highp float _365 = _111 - _289;
                    highp float _379 = -_267;
                    highp float _382 = sqrt(max(_274 - _201, 0.0));
                    highp float _393 = _111 - _266;
                    _409 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_289) * _350) + sqrt(max((_351 * ((_350 * _350) - 1.0)) + _200, 0.0)), 0.0) - _365) / ((_354 + _203) - _365)) * 0.99609375), 0.0078125 + ((_354 / _203) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_266) * _379) + sqrt(max((_274 * ((_379 * _379) - 1.0)) + _200, 0.0)), 0.0) - _393) / ((_382 + _203) - _393)) * 0.99609375), 0.0078125 + ((_382 / _203) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _298 = sqrt(max(_274 - _201, 0.0));
                    highp float _306 = _111 - _266;
                    highp float _320 = _289 * _289;
                    highp float _323 = sqrt(max(_320 - _201, 0.0));
                    highp float _334 = _111 - _289;
                    _409 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_266) * _267) + sqrt(max(_277 + _200, 0.0)), 0.0) - _306) / ((_298 + _203) - _306)) * 0.99609375), 0.0078125 + ((_298 / _203) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_289) * _292) + sqrt(max((_320 * ((_292 * _292) - 1.0)) + _200, 0.0)), 0.0) - _334) / ((_323 + _203) - _334)) * 0.99609375), 0.0078125 + ((_323 / _203) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _412 = sqrt(max(_274 - _201, 0.0));
        highp float _415 = 0.015625 + ((_412 / _203) * 0.96875);
        highp float _418 = ((_265 * _265) - _274) + _201;
        highp float _451;
        if (_280)
        {
            highp float _441 = _266 - u_fragParams.u_earthCenter.w;
            _451 = 0.5 - (0.5 * (0.0078125 + (((_412 == _441) ? 0.0 : ((((-_265) - sqrt(max(_418, 0.0))) - _441) / (_412 - _441))) * 0.984375)));
        }
        else
        {
            highp float _428 = _111 - _266;
            _451 = 0.5 + (0.5 * (0.0078125 + (((((-_265) + sqrt(max(_418 + (_203 * _203), 0.0))) - _428) / ((_412 + _203) - _428)) * 0.984375)));
        }
        highp float _456 = -u_fragParams.u_earthCenter.w;
        highp float _463 = _203 - _176;
        highp float _464 = (max((_456 * _269) + sqrt(max((_201 * ((_269 * _269) - 1.0)) + _200, 0.0)), 0.0) - _176) / _463;
        highp float _466 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _463;
        highp float _473 = 0.015625 + ((max(1.0 - (_464 / _466), 0.0) / (1.0 + _464)) * 0.96875);
        highp float _475 = (_270 + 1.0) * 3.5;
        highp float _476 = floor(_475);
        highp float _477 = _475 - _476;
        highp float _481 = _476 + 1.0;
        highp vec4 _487 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_476 + _473) * 0.125, _451, _415));
        highp float _488 = 1.0 - _477;
        highp vec4 _491 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_481 + _473) * 0.125, _451, _415));
        highp vec4 _493 = (_487 * _488) + (_491 * _477);
        highp vec3 _494 = _493.xyz;
        highp vec3 _507;
        switch (0u)
        {
            default:
            {
                highp float _497 = _493.x;
                if (_497 == 0.0)
                {
                    _507 = vec3(0.0);
                    break;
                }
                _507 = (((_494 * _493.w) / vec3(_497)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _508 = max(_272, 0.0);
        highp float _513 = clamp(sqrt((_508 * (_508 + _284)) + _274), u_fragParams.u_earthCenter.w, _111);
        highp float _514 = _265 + _508;
        highp float _517 = (_268 + (_508 * _270)) / _513;
        highp float _518 = _513 * _513;
        highp float _521 = sqrt(max(_518 - _201, 0.0));
        highp float _524 = 0.015625 + ((_521 / _203) * 0.96875);
        highp float _527 = ((_514 * _514) - _518) + _201;
        highp float _560;
        if (_280)
        {
            highp float _550 = _513 - u_fragParams.u_earthCenter.w;
            _560 = 0.5 - (0.5 * (0.0078125 + (((_521 == _550) ? 0.0 : ((((-_514) - sqrt(max(_527, 0.0))) - _550) / (_521 - _550))) * 0.984375)));
        }
        else
        {
            highp float _537 = _111 - _513;
            _560 = 0.5 + (0.5 * (0.0078125 + (((((-_514) + sqrt(max(_527 + (_203 * _203), 0.0))) - _537) / ((_521 + _203) - _537)) * 0.984375)));
        }
        highp float _571 = (max((_456 * _517) + sqrt(max((_201 * ((_517 * _517) - 1.0)) + _200, 0.0)), 0.0) - _176) / _463;
        highp float _578 = 0.015625 + ((max(1.0 - (_571 / _466), 0.0) / (1.0 + _571)) * 0.96875);
        highp vec4 _586 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_476 + _578) * 0.125, _560, _524));
        highp vec4 _589 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_481 + _578) * 0.125, _560, _524));
        highp vec4 _591 = (_586 * _488) + (_589 * _477);
        highp vec3 _592 = _591.xyz;
        highp vec3 _605;
        switch (0u)
        {
            default:
            {
                highp float _595 = _591.x;
                if (_595 == 0.0)
                {
                    _605 = vec3(0.0);
                    break;
                }
                _605 = (((_592 * _591.w) / vec3(_595)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _607 = _494 - (_409 * _592);
        highp vec3 _609 = _507 - (_409 * _605);
        highp float _610 = _609.x;
        highp float _611 = _607.x;
        highp vec3 _626;
        switch (0u)
        {
            default:
            {
                if (_611 == 0.0)
                {
                    _626 = vec3(0.0);
                    break;
                }
                _626 = (((vec4(_611, _607.yz, _610).xyz * _610) / vec3(_611)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _630 = 1.0 + (_270 * _270);
        _642 = (((_152.xyz * 0.3183098733425140380859375) * (((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_231.xyz * smoothstep(_194 * (-0.004674999974668025970458984375), _194 * 0.004674999974668025970458984375, _174 - (-sqrt(max(1.0 - (_194 * _194), 0.0)))))) * max(dot(_169, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_187.xyz * (1.0 + (dot(_169, _168) / _172))) * 0.5))) * _409) + ((_607 * (0.0596831031143665313720703125 * _630)) + ((_626 * smoothstep(0.0, 0.00999999977648258209228515625, _269)) * ((0.01627720706164836883544921875 * _630) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _270), 1.5))));
    }
    else
    {
        _642 = vec3(0.0);
    }
    highp float _644 = dot(_153, _124);
    highp float _646 = _644 * _644;
    highp float _648 = -_644;
    highp float _649 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _652 = _648 - sqrt(_649 - (dot(_153, _153) - _646));
    bool _653 = _652 > 0.0;
    highp vec3 _1242;
    if (_653)
    {
        highp vec3 _658 = (u_fragParams.u_camera.xyz + (_124 * _652)) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _659 = normalize(_658);
        highp float _662 = length(_658);
        highp float _664 = dot(_658, u_fragParams.u_sunDirection.xyz) / _662;
        highp float _666 = _111 - u_fragParams.u_earthCenter.w;
        highp vec4 _677 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_664 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_662 - u_fragParams.u_earthCenter.w) / _666) * 0.9375)));
        highp float _684 = u_fragParams.u_earthCenter.w / _662;
        highp float _690 = _111 * _111;
        highp float _692 = sqrt(_690 - _649);
        highp float _693 = _662 * _662;
        highp float _696 = sqrt(max(_693 - _649, 0.0));
        highp float _707 = _111 - _662;
        highp vec4 _720 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_662) * _664) + sqrt(max((_693 * ((_664 * _664) - 1.0)) + _690, 0.0)), 0.0) - _707) / ((_696 + _692) - _707)) * 0.99609375), 0.0078125 + ((_696 / _692) * 0.984375)));
        highp float _737 = max(0.0, min(0.0, _652)) * smoothstep(0.0199999995529651641845703125, 0.039999999105930328369140625, dot(normalize(_153), u_fragParams.u_sunDirection.xyz));
        highp vec3 _740 = normalize(_658 - _153);
        highp float _741 = length(_153);
        highp float _742 = dot(_153, _740);
        highp float _749 = (-_742) - sqrt(((_742 * _742) - (_741 * _741)) + _690);
        bool _750 = _749 > 0.0;
        highp vec3 _756;
        highp float _757;
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
        highp float _776;
        highp float _758 = _750 ? _111 : _741;
        highp float _759 = _757 / _758;
        highp float _760 = dot(_756, u_fragParams.u_sunDirection.xyz);
        highp float _761 = _760 / _758;
        highp float _762 = dot(_740, u_fragParams.u_sunDirection.xyz);
        highp float _764 = length(_658 - _756);
        highp float _766 = _758 * _758;
        highp float _769 = _766 * ((_759 * _759) - 1.0);
        bool _772 = (_759 < 0.0) && ((_769 + _649) >= 0.0);
        highp vec3 _901;
        switch (0u)
        {
            default:
            {
                _776 = (2.0 * _758) * _759;
                highp float _781 = clamp(sqrt((_764 * (_764 + _776)) + _766), u_fragParams.u_earthCenter.w, _111);
                highp float _784 = clamp((_757 + _764) / _781, -1.0, 1.0);
                if (_772)
                {
                    highp float _842 = -_784;
                    highp float _843 = _781 * _781;
                    highp float _846 = sqrt(max(_843 - _649, 0.0));
                    highp float _857 = _111 - _781;
                    highp float _871 = -_759;
                    highp float _874 = sqrt(max(_766 - _649, 0.0));
                    highp float _885 = _111 - _758;
                    _901 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_781) * _842) + sqrt(max((_843 * ((_842 * _842) - 1.0)) + _690, 0.0)), 0.0) - _857) / ((_846 + _692) - _857)) * 0.99609375), 0.0078125 + ((_846 / _692) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_758) * _871) + sqrt(max((_766 * ((_871 * _871) - 1.0)) + _690, 0.0)), 0.0) - _885) / ((_874 + _692) - _885)) * 0.99609375), 0.0078125 + ((_874 / _692) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _790 = sqrt(max(_766 - _649, 0.0));
                    highp float _798 = _111 - _758;
                    highp float _812 = _781 * _781;
                    highp float _815 = sqrt(max(_812 - _649, 0.0));
                    highp float _826 = _111 - _781;
                    _901 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_758) * _759) + sqrt(max(_769 + _690, 0.0)), 0.0) - _798) / ((_790 + _692) - _798)) * 0.99609375), 0.0078125 + ((_790 / _692) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_781) * _784) + sqrt(max((_812 * ((_784 * _784) - 1.0)) + _690, 0.0)), 0.0) - _826) / ((_815 + _692) - _826)) * 0.99609375), 0.0078125 + ((_815 / _692) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _904 = sqrt(max(_766 - _649, 0.0));
        highp float _905 = _904 / _692;
        highp float _907 = 0.015625 + (_905 * 0.96875);
        highp float _910 = ((_757 * _757) - _766) + _649;
        highp float _943;
        if (_772)
        {
            highp float _933 = _758 - u_fragParams.u_earthCenter.w;
            _943 = 0.5 - (0.5 * (0.0078125 + (((_904 == _933) ? 0.0 : ((((-_757) - sqrt(max(_910, 0.0))) - _933) / (_904 - _933))) * 0.984375)));
        }
        else
        {
            highp float _920 = _111 - _758;
            _943 = 0.5 + (0.5 * (0.0078125 + (((((-_757) + sqrt(max(_910 + (_692 * _692), 0.0))) - _920) / ((_904 + _692) - _920)) * 0.984375)));
        }
        highp float _948 = -u_fragParams.u_earthCenter.w;
        highp float _955 = _692 - _666;
        highp float _956 = (max((_948 * _761) + sqrt(max((_649 * ((_761 * _761) - 1.0)) + _690, 0.0)), 0.0) - _666) / _955;
        highp float _958 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _955;
        highp float _965 = 0.015625 + ((max(1.0 - (_956 / _958), 0.0) / (1.0 + _956)) * 0.96875);
        highp float _967 = (_762 + 1.0) * 3.5;
        highp float _968 = floor(_967);
        highp float _969 = _967 - _968;
        highp float _973 = _968 + 1.0;
        highp vec4 _979 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_968 + _965) * 0.125, _943, _907));
        highp float _980 = 1.0 - _969;
        highp vec4 _983 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_973 + _965) * 0.125, _943, _907));
        highp vec4 _985 = (_979 * _980) + (_983 * _969);
        highp vec3 _986 = _985.xyz;
        highp vec3 _999;
        switch (0u)
        {
            default:
            {
                highp float _989 = _985.x;
                if (_989 == 0.0)
                {
                    _999 = vec3(0.0);
                    break;
                }
                _999 = (((_986 * _985.w) / vec3(_989)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1001 = max(_764 - _737, 0.0);
        highp float _1006 = clamp(sqrt((_1001 * (_1001 + _776)) + _766), u_fragParams.u_earthCenter.w, _111);
        highp float _1007 = _757 + _1001;
        highp float _1010 = (_760 + (_1001 * _762)) / _1006;
        highp float _1011 = _1006 * _1006;
        highp float _1014 = sqrt(max(_1011 - _649, 0.0));
        highp float _1015 = _1014 / _692;
        highp float _1017 = 0.015625 + (_1015 * 0.96875);
        highp float _1020 = ((_1007 * _1007) - _1011) + _649;
        highp float _1053;
        if (_772)
        {
            highp float _1043 = _1006 - u_fragParams.u_earthCenter.w;
            _1053 = 0.5 - (0.5 * (0.0078125 + (((_1014 == _1043) ? 0.0 : ((((-_1007) - sqrt(max(_1020, 0.0))) - _1043) / (_1014 - _1043))) * 0.984375)));
        }
        else
        {
            highp float _1030 = _111 - _1006;
            _1053 = 0.5 + (0.5 * (0.0078125 + (((((-_1007) + sqrt(max(_1020 + (_692 * _692), 0.0))) - _1030) / ((_1014 + _692) - _1030)) * 0.984375)));
        }
        highp float _1064 = (max((_948 * _1010) + sqrt(max((_649 * ((_1010 * _1010) - 1.0)) + _690, 0.0)), 0.0) - _666) / _955;
        highp float _1071 = 0.015625 + ((max(1.0 - (_1064 / _958), 0.0) / (1.0 + _1064)) * 0.96875);
        highp vec4 _1079 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_968 + _1071) * 0.125, _1053, _1017));
        highp vec4 _1082 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_973 + _1071) * 0.125, _1053, _1017));
        highp vec4 _1084 = (_1079 * _980) + (_1082 * _969);
        highp vec3 _1085 = _1084.xyz;
        highp vec3 _1098;
        switch (0u)
        {
            default:
            {
                highp float _1088 = _1084.x;
                if (_1088 == 0.0)
                {
                    _1098 = vec3(0.0);
                    break;
                }
                _1098 = (((_1085 * _1084.w) / vec3(_1088)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _1205;
        if (_737 > 0.0)
        {
            highp vec3 _1204;
            switch (0u)
            {
                default:
                {
                    highp float _1105 = clamp(_1007 / _1006, -1.0, 1.0);
                    if (_772)
                    {
                        highp float _1154 = -_1105;
                        highp float _1165 = _111 - _1006;
                        highp float _1178 = -_759;
                        highp float _1189 = _111 - _758;
                        _1204 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1006) * _1154) + sqrt(max((_1011 * ((_1154 * _1154) - 1.0)) + _690, 0.0)), 0.0) - _1165) / ((_1014 + _692) - _1165)) * 0.99609375), 0.0078125 + (_1015 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_758) * _1178) + sqrt(max((_766 * ((_1178 * _1178) - 1.0)) + _690, 0.0)), 0.0) - _1189) / ((_904 + _692) - _1189)) * 0.99609375), 0.0078125 + (_905 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _1116 = _111 - _758;
                        highp float _1139 = _111 - _1006;
                        _1204 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_758) * _759) + sqrt(max(_769 + _690, 0.0)), 0.0) - _1116) / ((_904 + _692) - _1116)) * 0.99609375), 0.0078125 + (_905 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1006) * _1105) + sqrt(max((_1011 * ((_1105 * _1105) - 1.0)) + _690, 0.0)), 0.0) - _1139) / ((_1014 + _692) - _1139)) * 0.99609375), 0.0078125 + (_1015 * 0.984375))).xyz, vec3(1.0));
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
        highp vec3 _1207 = _986 - (_1205 * _1085);
        highp vec3 _1209 = _999 - (_1205 * _1098);
        highp float _1210 = _1209.x;
        highp float _1211 = _1207.x;
        highp vec3 _1226;
        switch (0u)
        {
            default:
            {
                if (_1211 == 0.0)
                {
                    _1226 = vec3(0.0);
                    break;
                }
                _1226 = (((vec4(_1211, _1207.yz, _1210).xyz * _1210) / vec3(_1211)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1230 = 1.0 + (_762 * _762);
        _1242 = (((_152.xyz * 0.3183098733425140380859375) * (((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * (_720.xyz * smoothstep(_684 * (-0.004674999974668025970458984375), _684 * 0.004674999974668025970458984375, _664 - (-sqrt(max(1.0 - (_684 * _684), 0.0)))))) * max(dot(_659, u_fragParams.u_sunDirection.xyz), 0.0)) + ((_677.xyz * (1.0 + (dot(_659, _658) / _662))) * 0.5))) * _901) + ((_1207 * (0.0596831031143665313720703125 * _1230)) + ((_1226 * smoothstep(0.0, 0.00999999977648258209228515625, _761)) * ((0.01627720706164836883544921875 * _1230) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _762), 1.5))));
    }
    else
    {
        _1242 = vec3(0.0);
    }
    highp vec3 _1412;
    highp vec3 _1413;
    switch (0u)
    {
        default:
        {
            highp float _1248 = length(_153);
            highp float _1251 = _111 * _111;
            highp float _1254 = _648 - sqrt((_646 - (_1248 * _1248)) + _1251);
            bool _1255 = _1254 > 0.0;
            highp vec3 _1265;
            highp float _1266;
            if (_1255)
            {
                _1265 = _153 + (_124 * _1254);
                _1266 = _644 + _1254;
            }
            else
            {
                if (_1248 > _111)
                {
                    _1412 = vec3(1.0);
                    _1413 = vec3(0.0);
                    break;
                }
                _1265 = _153;
                _1266 = _644;
            }
            highp float _1267 = _1255 ? _111 : _1248;
            highp float _1268 = _1266 / _1267;
            highp float _1270 = dot(_1265, u_fragParams.u_sunDirection.xyz) / _1267;
            highp float _1271 = dot(_124, u_fragParams.u_sunDirection.xyz);
            highp float _1273 = _1267 * _1267;
            highp float _1276 = _1273 * ((_1268 * _1268) - 1.0);
            bool _1279 = (_1268 < 0.0) && ((_1276 + _649) >= 0.0);
            highp float _1281 = sqrt(_1251 - _649);
            highp float _1284 = sqrt(max(_1273 - _649, 0.0));
            highp float _1292 = _111 - _1267;
            highp float _1295 = (_1284 + _1281) - _1292;
            highp float _1297 = _1284 / _1281;
            highp vec4 _1305 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1267) * _1268) + sqrt(max(_1276 + _1251, 0.0)), 0.0) - _1292) / _1295) * 0.99609375), 0.0078125 + (_1297 * 0.984375)));
            highp vec3 _1306 = _1305.xyz;
            bvec3 _1307 = bvec3(_1279);
            highp float _1310 = 0.015625 + (_1297 * 0.96875);
            highp float _1313 = ((_1266 * _1266) - _1273) + _649;
            highp float _1343;
            if (_1279)
            {
                highp float _1333 = _1267 - u_fragParams.u_earthCenter.w;
                _1343 = 0.5 - (0.5 * (0.0078125 + (((_1284 == _1333) ? 0.0 : ((((-_1266) - sqrt(max(_1313, 0.0))) - _1333) / (_1284 - _1333))) * 0.984375)));
            }
            else
            {
                _1343 = 0.5 + (0.5 * (0.0078125 + (((((-_1266) + sqrt(max(_1313 + (_1281 * _1281), 0.0))) - _1292) / _1295) * 0.984375)));
            }
            highp float _1354 = _111 - u_fragParams.u_earthCenter.w;
            highp float _1356 = _1281 - _1354;
            highp float _1357 = (max(((-u_fragParams.u_earthCenter.w) * _1270) + sqrt(max((_649 * ((_1270 * _1270) - 1.0)) + _1251, 0.0)), 0.0) - _1354) / _1356;
            highp float _1366 = 0.015625 + ((max(1.0 - (_1357 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1356)), 0.0) / (1.0 + _1357)) * 0.96875);
            highp float _1368 = (_1271 + 1.0) * 3.5;
            highp float _1369 = floor(_1368);
            highp float _1370 = _1368 - _1369;
            highp vec4 _1380 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1369 + _1366) * 0.125, _1343, _1310));
            highp vec4 _1384 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1369 + 1.0) + _1366) * 0.125, _1343, _1310));
            highp vec4 _1386 = (_1380 * (1.0 - _1370)) + (_1384 * _1370);
            highp vec3 _1387 = _1386.xyz;
            highp vec3 _1400;
            switch (0u)
            {
                default:
                {
                    highp float _1390 = _1386.x;
                    if (_1390 == 0.0)
                    {
                        _1400 = vec3(0.0);
                        break;
                    }
                    _1400 = (((_1387 * _1386.w) / vec3(_1390)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1402 = 1.0 + (_1271 * _1271);
            _1412 = vec3(_1307.x ? vec3(0.0).x : _1306.x, _1307.y ? vec3(0.0).y : _1306.y, _1307.z ? vec3(0.0).z : _1306.z);
            _1413 = (_1387 * (0.0596831031143665313720703125 * _1402)) + (_1400 * ((0.01627720706164836883544921875 * _1402) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1271), 1.5)));
            break;
        }
    }
    highp vec3 _1421;
    if (dot(_124, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1421 = _1413 + (_1412 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1421 = _1413;
    }
    highp vec3 _1435 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1421, _1242, vec3(float(_653))), _642, vec3(float(_163)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1437 = vec4(_1435.x, _1435.y, _1435.z, _103.w);
    _1437.w = 1.0;
    out_var_SV_Target = _1437;
    gl_FragDepth = _129;
}

