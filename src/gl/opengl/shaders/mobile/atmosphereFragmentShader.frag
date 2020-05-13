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
    layout(row_major) highp mat4 u_inverseViewProjection;
    layout(row_major) highp mat4 u_inverseProjection;
} u_fragParams;

uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform highp sampler2D SPIRV_Cross_CombinedirradianceTextureirradianceSampler;
uniform highp sampler2D SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler;
uniform highp sampler3D SPIRV_Cross_CombinedscatteringTexturescatteringSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec3 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

vec4 _109;

void main()
{
    highp float _117 = u_fragParams.u_earthCenter.w + 60000.0;
    highp vec3 _130 = normalize(varying_TEXCOORD1);
    highp vec4 _134 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0);
    highp vec4 _138 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, varying_TEXCOORD0);
    highp vec4 _142 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0);
    highp float _143 = _142.x;
    highp float _148 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp vec3 _162 = pow(abs(_134.xyz), vec3(2.2000000476837158203125));
    highp float _168 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _148) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _143 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _148))) * u_cameraPlaneParams.s_CameraFarPlane;
    bool _171 = _143 < 0.64999997615814208984375;
    highp vec3 _762;
    if (_171)
    {
        highp vec3 _174 = (u_fragParams.u_camera.xyz + (_130 * _168)) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _175 = normalize(_174);
        highp float _178 = length(_174);
        highp float _180 = dot(_174, u_fragParams.u_sunDirection.xyz) / _178;
        highp float _182 = _117 - u_fragParams.u_earthCenter.w;
        highp vec4 _193 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_180 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_178 - u_fragParams.u_earthCenter.w) / _182) * 0.9375)));
        highp float _200 = u_fragParams.u_earthCenter.w / _178;
        highp float _206 = _117 * _117;
        highp float _207 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        highp float _209 = sqrt(_206 - _207);
        highp float _210 = _178 * _178;
        highp float _213 = sqrt(max(_210 - _207, 0.0));
        highp float _224 = _117 - _178;
        highp vec4 _237 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_178) * _180) + sqrt(max((_210 * ((_180 * _180) - 1.0)) + _206, 0.0)), 0.0) - _224) / ((_213 + _209) - _224)) * 0.99609375), 0.0078125 + ((_213 / _209) * 0.984375)));
        highp float _256 = mix(_168 * 0.64999997615814208984375, 0.0, pow(_143 * 1.53846156597137451171875, 8.0));
        highp vec3 _257 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
        highp vec3 _260 = normalize(_174 - _257);
        highp float _261 = length(_257);
        highp float _262 = dot(_257, _260);
        highp float _269 = (-_262) - sqrt(((_262 * _262) - (_261 * _261)) + _206);
        bool _270 = _269 > 0.0;
        highp vec3 _276;
        highp float _277;
        if (_270)
        {
            _276 = _257 + (_260 * _269);
            _277 = _262 + _269;
        }
        else
        {
            _276 = _257;
            _277 = _262;
        }
        highp float _296;
        highp float _278 = _270 ? _117 : _261;
        highp float _279 = _277 / _278;
        highp float _280 = dot(_276, u_fragParams.u_sunDirection.xyz);
        highp float _281 = _280 / _278;
        highp float _282 = dot(_260, u_fragParams.u_sunDirection.xyz);
        highp float _284 = length(_174 - _276);
        highp float _286 = _278 * _278;
        highp float _289 = _286 * ((_279 * _279) - 1.0);
        bool _292 = (_279 < 0.0) && ((_289 + _207) >= 0.0);
        highp vec3 _421;
        switch (0u)
        {
            case 0u:
            {
                _296 = (2.0 * _278) * _279;
                highp float _301 = clamp(sqrt((_284 * (_284 + _296)) + _286), u_fragParams.u_earthCenter.w, _117);
                highp float _304 = clamp((_277 + _284) / _301, -1.0, 1.0);
                if (_292)
                {
                    highp float _362 = -_304;
                    highp float _363 = _301 * _301;
                    highp float _366 = sqrt(max(_363 - _207, 0.0));
                    highp float _377 = _117 - _301;
                    highp float _391 = -_279;
                    highp float _394 = sqrt(max(_286 - _207, 0.0));
                    highp float _405 = _117 - _278;
                    _421 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_301) * _362) + sqrt(max((_363 * ((_362 * _362) - 1.0)) + _206, 0.0)), 0.0) - _377) / ((_366 + _209) - _377)) * 0.99609375), 0.0078125 + ((_366 / _209) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_278) * _391) + sqrt(max((_286 * ((_391 * _391) - 1.0)) + _206, 0.0)), 0.0) - _405) / ((_394 + _209) - _405)) * 0.99609375), 0.0078125 + ((_394 / _209) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _310 = sqrt(max(_286 - _207, 0.0));
                    highp float _318 = _117 - _278;
                    highp float _332 = _301 * _301;
                    highp float _335 = sqrt(max(_332 - _207, 0.0));
                    highp float _346 = _117 - _301;
                    _421 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_278) * _279) + sqrt(max(_289 + _206, 0.0)), 0.0) - _318) / ((_310 + _209) - _318)) * 0.99609375), 0.0078125 + ((_310 / _209) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_301) * _304) + sqrt(max((_332 * ((_304 * _304) - 1.0)) + _206, 0.0)), 0.0) - _346) / ((_335 + _209) - _346)) * 0.99609375), 0.0078125 + ((_335 / _209) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _424 = sqrt(max(_286 - _207, 0.0));
        highp float _425 = _424 / _209;
        highp float _427 = 0.015625 + (_425 * 0.96875);
        highp float _430 = ((_277 * _277) - _286) + _207;
        highp float _463;
        if (_292)
        {
            highp float _453 = _278 - u_fragParams.u_earthCenter.w;
            _463 = 0.5 - (0.5 * (0.0078125 + (((_424 == _453) ? 0.0 : ((((-_277) - sqrt(max(_430, 0.0))) - _453) / (_424 - _453))) * 0.984375)));
        }
        else
        {
            highp float _440 = _117 - _278;
            _463 = 0.5 + (0.5 * (0.0078125 + (((((-_277) + sqrt(max(_430 + (_209 * _209), 0.0))) - _440) / ((_424 + _209) - _440)) * 0.984375)));
        }
        highp float _468 = -u_fragParams.u_earthCenter.w;
        highp float _475 = _209 - _182;
        highp float _476 = (max((_468 * _281) + sqrt(max((_207 * ((_281 * _281) - 1.0)) + _206, 0.0)), 0.0) - _182) / _475;
        highp float _478 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _475;
        highp float _485 = 0.015625 + ((max(1.0 - (_476 / _478), 0.0) / (1.0 + _476)) * 0.96875);
        highp float _487 = (_282 + 1.0) * 3.5;
        highp float _488 = floor(_487);
        highp float _489 = _487 - _488;
        highp float _493 = _488 + 1.0;
        highp vec4 _499 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_488 + _485) * 0.125, _463, _427));
        highp float _500 = 1.0 - _489;
        highp vec4 _503 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_493 + _485) * 0.125, _463, _427));
        highp vec4 _505 = (_499 * _500) + (_503 * _489);
        highp vec3 _506 = _505.xyz;
        highp vec3 _519;
        switch (0u)
        {
            case 0u:
            {
                highp float _509 = _505.x;
                if (_509 == 0.0)
                {
                    _519 = vec3(0.0);
                    break;
                }
                _519 = (((_506 * _505.w) / vec3(_509)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _521 = max(_284 - _256, 0.0);
        highp float _526 = clamp(sqrt((_521 * (_521 + _296)) + _286), u_fragParams.u_earthCenter.w, _117);
        highp float _527 = _277 + _521;
        highp float _530 = (_280 + (_521 * _282)) / _526;
        highp float _531 = _526 * _526;
        highp float _534 = sqrt(max(_531 - _207, 0.0));
        highp float _535 = _534 / _209;
        highp float _537 = 0.015625 + (_535 * 0.96875);
        highp float _540 = ((_527 * _527) - _531) + _207;
        highp float _573;
        if (_292)
        {
            highp float _563 = _526 - u_fragParams.u_earthCenter.w;
            _573 = 0.5 - (0.5 * (0.0078125 + (((_534 == _563) ? 0.0 : ((((-_527) - sqrt(max(_540, 0.0))) - _563) / (_534 - _563))) * 0.984375)));
        }
        else
        {
            highp float _550 = _117 - _526;
            _573 = 0.5 + (0.5 * (0.0078125 + (((((-_527) + sqrt(max(_540 + (_209 * _209), 0.0))) - _550) / ((_534 + _209) - _550)) * 0.984375)));
        }
        highp float _584 = (max((_468 * _530) + sqrt(max((_207 * ((_530 * _530) - 1.0)) + _206, 0.0)), 0.0) - _182) / _475;
        highp float _591 = 0.015625 + ((max(1.0 - (_584 / _478), 0.0) / (1.0 + _584)) * 0.96875);
        highp vec4 _599 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_488 + _591) * 0.125, _573, _537));
        highp vec4 _602 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_493 + _591) * 0.125, _573, _537));
        highp vec4 _604 = (_599 * _500) + (_602 * _489);
        highp vec3 _605 = _604.xyz;
        highp vec3 _618;
        switch (0u)
        {
            case 0u:
            {
                highp float _608 = _604.x;
                if (_608 == 0.0)
                {
                    _618 = vec3(0.0);
                    break;
                }
                _618 = (((_605 * _604.w) / vec3(_608)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _725;
        if (_256 > 0.0)
        {
            highp vec3 _724;
            switch (0u)
            {
                case 0u:
                {
                    highp float _625 = clamp(_527 / _526, -1.0, 1.0);
                    if (_292)
                    {
                        highp float _674 = -_625;
                        highp float _685 = _117 - _526;
                        highp float _698 = -_279;
                        highp float _709 = _117 - _278;
                        _724 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_526) * _674) + sqrt(max((_531 * ((_674 * _674) - 1.0)) + _206, 0.0)), 0.0) - _685) / ((_534 + _209) - _685)) * 0.99609375), 0.0078125 + (_535 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_278) * _698) + sqrt(max((_286 * ((_698 * _698) - 1.0)) + _206, 0.0)), 0.0) - _709) / ((_424 + _209) - _709)) * 0.99609375), 0.0078125 + (_425 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _636 = _117 - _278;
                        highp float _659 = _117 - _526;
                        _724 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_278) * _279) + sqrt(max(_289 + _206, 0.0)), 0.0) - _636) / ((_424 + _209) - _636)) * 0.99609375), 0.0078125 + (_425 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_526) * _625) + sqrt(max((_531 * ((_625 * _625) - 1.0)) + _206, 0.0)), 0.0) - _659) / ((_534 + _209) - _659)) * 0.99609375), 0.0078125 + (_535 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _725 = _724;
        }
        else
        {
            _725 = _421;
        }
        highp vec3 _727 = _506 - (_725 * _605);
        highp vec3 _729 = _519 - (_725 * _618);
        highp float _730 = _729.x;
        highp float _731 = _727.x;
        highp vec3 _746;
        switch (0u)
        {
            case 0u:
            {
                if (_731 == 0.0)
                {
                    _746 = vec3(0.0);
                    break;
                }
                _746 = (((vec4(_731, _727.yz, _730).xyz * _730) / vec3(_731)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _750 = 1.0 + (_282 * _282);
        _762 = (((_162.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_237.xyz * smoothstep(_200 * (-0.004674999974668025970458984375), _200 * 0.004674999974668025970458984375, _180 - (-sqrt(max(1.0 - (_200 * _200), 0.0))))) * max(dot(_175, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_193.xyz * (1.0 + (dot(_175, _174) / _178))) * 0.5))) * _421) + ((_727 * (0.0596831031143665313720703125 * _750)) + ((_746 * smoothstep(0.0, 0.00999999977648258209228515625, _281)) * ((0.01627720706164836883544921875 * _750) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _282), 1.5))));
    }
    else
    {
        _762 = vec3(0.0);
    }
    highp vec3 _764 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    highp float _765 = dot(_764, _130);
    highp float _767 = _765 * _765;
    highp float _769 = -_765;
    highp float _770 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _773 = _769 - sqrt(_770 - (dot(_764, _764) - _767));
    bool _774 = _773 > 0.0;
    highp vec3 _1253;
    if (_774)
    {
        highp vec3 _779 = (u_fragParams.u_camera.xyz + (_130 * _773)) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _780 = normalize(_779);
        highp float _783 = length(_779);
        highp float _785 = dot(_779, u_fragParams.u_sunDirection.xyz) / _783;
        highp float _787 = _117 - u_fragParams.u_earthCenter.w;
        highp vec4 _798 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_785 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_783 - u_fragParams.u_earthCenter.w) / _787) * 0.9375)));
        highp float _805 = u_fragParams.u_earthCenter.w / _783;
        highp float _811 = _117 * _117;
        highp float _813 = sqrt(_811 - _770);
        highp float _814 = _783 * _783;
        highp float _817 = sqrt(max(_814 - _770, 0.0));
        highp float _828 = _117 - _783;
        highp vec4 _841 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_783) * _785) + sqrt(max((_814 * ((_785 * _785) - 1.0)) + _811, 0.0)), 0.0) - _828) / ((_817 + _813) - _828)) * 0.99609375), 0.0078125 + ((_817 / _813) * 0.984375)));
        highp vec3 _859 = normalize(_779 - _764);
        highp float _860 = length(_764);
        highp float _861 = dot(_764, _859);
        highp float _868 = (-_861) - sqrt(((_861 * _861) - (_860 * _860)) + _811);
        bool _869 = _868 > 0.0;
        highp vec3 _875;
        highp float _876;
        if (_869)
        {
            _875 = _764 + (_859 * _868);
            _876 = _861 + _868;
        }
        else
        {
            _875 = _764;
            _876 = _861;
        }
        highp float _895;
        highp float _877 = _869 ? _117 : _860;
        highp float _878 = _876 / _877;
        highp float _879 = dot(_875, u_fragParams.u_sunDirection.xyz);
        highp float _880 = _879 / _877;
        highp float _881 = dot(_859, u_fragParams.u_sunDirection.xyz);
        highp float _883 = length(_779 - _875);
        highp float _885 = _877 * _877;
        highp float _888 = _885 * ((_878 * _878) - 1.0);
        bool _891 = (_878 < 0.0) && ((_888 + _770) >= 0.0);
        highp vec3 _1020;
        switch (0u)
        {
            case 0u:
            {
                _895 = (2.0 * _877) * _878;
                highp float _900 = clamp(sqrt((_883 * (_883 + _895)) + _885), u_fragParams.u_earthCenter.w, _117);
                highp float _903 = clamp((_876 + _883) / _900, -1.0, 1.0);
                if (_891)
                {
                    highp float _961 = -_903;
                    highp float _962 = _900 * _900;
                    highp float _965 = sqrt(max(_962 - _770, 0.0));
                    highp float _976 = _117 - _900;
                    highp float _990 = -_878;
                    highp float _993 = sqrt(max(_885 - _770, 0.0));
                    highp float _1004 = _117 - _877;
                    _1020 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_900) * _961) + sqrt(max((_962 * ((_961 * _961) - 1.0)) + _811, 0.0)), 0.0) - _976) / ((_965 + _813) - _976)) * 0.99609375), 0.0078125 + ((_965 / _813) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_877) * _990) + sqrt(max((_885 * ((_990 * _990) - 1.0)) + _811, 0.0)), 0.0) - _1004) / ((_993 + _813) - _1004)) * 0.99609375), 0.0078125 + ((_993 / _813) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _909 = sqrt(max(_885 - _770, 0.0));
                    highp float _917 = _117 - _877;
                    highp float _931 = _900 * _900;
                    highp float _934 = sqrt(max(_931 - _770, 0.0));
                    highp float _945 = _117 - _900;
                    _1020 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_877) * _878) + sqrt(max(_888 + _811, 0.0)), 0.0) - _917) / ((_909 + _813) - _917)) * 0.99609375), 0.0078125 + ((_909 / _813) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_900) * _903) + sqrt(max((_931 * ((_903 * _903) - 1.0)) + _811, 0.0)), 0.0) - _945) / ((_934 + _813) - _945)) * 0.99609375), 0.0078125 + ((_934 / _813) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _1023 = sqrt(max(_885 - _770, 0.0));
        highp float _1026 = 0.015625 + ((_1023 / _813) * 0.96875);
        highp float _1029 = ((_876 * _876) - _885) + _770;
        highp float _1062;
        if (_891)
        {
            highp float _1052 = _877 - u_fragParams.u_earthCenter.w;
            _1062 = 0.5 - (0.5 * (0.0078125 + (((_1023 == _1052) ? 0.0 : ((((-_876) - sqrt(max(_1029, 0.0))) - _1052) / (_1023 - _1052))) * 0.984375)));
        }
        else
        {
            highp float _1039 = _117 - _877;
            _1062 = 0.5 + (0.5 * (0.0078125 + (((((-_876) + sqrt(max(_1029 + (_813 * _813), 0.0))) - _1039) / ((_1023 + _813) - _1039)) * 0.984375)));
        }
        highp float _1067 = -u_fragParams.u_earthCenter.w;
        highp float _1074 = _813 - _787;
        highp float _1075 = (max((_1067 * _880) + sqrt(max((_770 * ((_880 * _880) - 1.0)) + _811, 0.0)), 0.0) - _787) / _1074;
        highp float _1077 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1074;
        highp float _1084 = 0.015625 + ((max(1.0 - (_1075 / _1077), 0.0) / (1.0 + _1075)) * 0.96875);
        highp float _1086 = (_881 + 1.0) * 3.5;
        highp float _1087 = floor(_1086);
        highp float _1088 = _1086 - _1087;
        highp float _1092 = _1087 + 1.0;
        highp vec4 _1098 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1087 + _1084) * 0.125, _1062, _1026));
        highp float _1099 = 1.0 - _1088;
        highp vec4 _1102 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1092 + _1084) * 0.125, _1062, _1026));
        highp vec4 _1104 = (_1098 * _1099) + (_1102 * _1088);
        highp vec3 _1105 = _1104.xyz;
        highp vec3 _1118;
        switch (0u)
        {
            case 0u:
            {
                highp float _1108 = _1104.x;
                if (_1108 == 0.0)
                {
                    _1118 = vec3(0.0);
                    break;
                }
                _1118 = (((_1105 * _1104.w) / vec3(_1108)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1119 = max(_883, 0.0);
        highp float _1124 = clamp(sqrt((_1119 * (_1119 + _895)) + _885), u_fragParams.u_earthCenter.w, _117);
        highp float _1125 = _876 + _1119;
        highp float _1128 = (_879 + (_1119 * _881)) / _1124;
        highp float _1129 = _1124 * _1124;
        highp float _1132 = sqrt(max(_1129 - _770, 0.0));
        highp float _1135 = 0.015625 + ((_1132 / _813) * 0.96875);
        highp float _1138 = ((_1125 * _1125) - _1129) + _770;
        highp float _1171;
        if (_891)
        {
            highp float _1161 = _1124 - u_fragParams.u_earthCenter.w;
            _1171 = 0.5 - (0.5 * (0.0078125 + (((_1132 == _1161) ? 0.0 : ((((-_1125) - sqrt(max(_1138, 0.0))) - _1161) / (_1132 - _1161))) * 0.984375)));
        }
        else
        {
            highp float _1148 = _117 - _1124;
            _1171 = 0.5 + (0.5 * (0.0078125 + (((((-_1125) + sqrt(max(_1138 + (_813 * _813), 0.0))) - _1148) / ((_1132 + _813) - _1148)) * 0.984375)));
        }
        highp float _1182 = (max((_1067 * _1128) + sqrt(max((_770 * ((_1128 * _1128) - 1.0)) + _811, 0.0)), 0.0) - _787) / _1074;
        highp float _1189 = 0.015625 + ((max(1.0 - (_1182 / _1077), 0.0) / (1.0 + _1182)) * 0.96875);
        highp vec4 _1197 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1087 + _1189) * 0.125, _1171, _1135));
        highp vec4 _1200 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1092 + _1189) * 0.125, _1171, _1135));
        highp vec4 _1202 = (_1197 * _1099) + (_1200 * _1088);
        highp vec3 _1203 = _1202.xyz;
        highp vec3 _1216;
        switch (0u)
        {
            case 0u:
            {
                highp float _1206 = _1202.x;
                if (_1206 == 0.0)
                {
                    _1216 = vec3(0.0);
                    break;
                }
                _1216 = (((_1203 * _1202.w) / vec3(_1206)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _1218 = _1105 - (_1020 * _1203);
        highp vec3 _1220 = _1118 - (_1020 * _1216);
        highp float _1221 = _1220.x;
        highp float _1222 = _1218.x;
        highp vec3 _1237;
        switch (0u)
        {
            case 0u:
            {
                if (_1222 == 0.0)
                {
                    _1237 = vec3(0.0);
                    break;
                }
                _1237 = (((vec4(_1222, _1218.yz, _1221).xyz * _1221) / vec3(_1222)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1241 = 1.0 + (_881 * _881);
        _1253 = (((_162.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_841.xyz * smoothstep(_805 * (-0.004674999974668025970458984375), _805 * 0.004674999974668025970458984375, _785 - (-sqrt(max(1.0 - (_805 * _805), 0.0))))) * max(dot(_780, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_798.xyz * (1.0 + (dot(_780, _779) / _783))) * 0.5))) * _1020) + ((_1218 * (0.0596831031143665313720703125 * _1241)) + ((_1237 * smoothstep(0.0, 0.00999999977648258209228515625, _880)) * ((0.01627720706164836883544921875 * _1241) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _881), 1.5))));
    }
    else
    {
        _1253 = vec3(0.0);
    }
    highp vec3 _1423;
    highp vec3 _1424;
    switch (0u)
    {
        case 0u:
        {
            highp float _1259 = length(_764);
            highp float _1262 = _117 * _117;
            highp float _1265 = _769 - sqrt((_767 - (_1259 * _1259)) + _1262);
            bool _1266 = _1265 > 0.0;
            highp vec3 _1276;
            highp float _1277;
            if (_1266)
            {
                _1276 = _764 + (_130 * _1265);
                _1277 = _765 + _1265;
            }
            else
            {
                if (_1259 > _117)
                {
                    _1423 = vec3(1.0);
                    _1424 = vec3(0.0);
                    break;
                }
                _1276 = _764;
                _1277 = _765;
            }
            highp float _1278 = _1266 ? _117 : _1259;
            highp float _1279 = _1277 / _1278;
            highp float _1281 = dot(_1276, u_fragParams.u_sunDirection.xyz) / _1278;
            highp float _1282 = dot(_130, u_fragParams.u_sunDirection.xyz);
            highp float _1284 = _1278 * _1278;
            highp float _1287 = _1284 * ((_1279 * _1279) - 1.0);
            bool _1290 = (_1279 < 0.0) && ((_1287 + _770) >= 0.0);
            highp float _1292 = sqrt(_1262 - _770);
            highp float _1295 = sqrt(max(_1284 - _770, 0.0));
            highp float _1303 = _117 - _1278;
            highp float _1306 = (_1295 + _1292) - _1303;
            highp float _1308 = _1295 / _1292;
            highp vec4 _1316 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1278) * _1279) + sqrt(max(_1287 + _1262, 0.0)), 0.0) - _1303) / _1306) * 0.99609375), 0.0078125 + (_1308 * 0.984375)));
            highp vec3 _1317 = _1316.xyz;
            bvec3 _1318 = bvec3(_1290);
            highp float _1321 = 0.015625 + (_1308 * 0.96875);
            highp float _1324 = ((_1277 * _1277) - _1284) + _770;
            highp float _1354;
            if (_1290)
            {
                highp float _1344 = _1278 - u_fragParams.u_earthCenter.w;
                _1354 = 0.5 - (0.5 * (0.0078125 + (((_1295 == _1344) ? 0.0 : ((((-_1277) - sqrt(max(_1324, 0.0))) - _1344) / (_1295 - _1344))) * 0.984375)));
            }
            else
            {
                _1354 = 0.5 + (0.5 * (0.0078125 + (((((-_1277) + sqrt(max(_1324 + (_1292 * _1292), 0.0))) - _1303) / _1306) * 0.984375)));
            }
            highp float _1365 = _117 - u_fragParams.u_earthCenter.w;
            highp float _1367 = _1292 - _1365;
            highp float _1368 = (max(((-u_fragParams.u_earthCenter.w) * _1281) + sqrt(max((_770 * ((_1281 * _1281) - 1.0)) + _1262, 0.0)), 0.0) - _1365) / _1367;
            highp float _1377 = 0.015625 + ((max(1.0 - (_1368 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1367)), 0.0) / (1.0 + _1368)) * 0.96875);
            highp float _1379 = (_1282 + 1.0) * 3.5;
            highp float _1380 = floor(_1379);
            highp float _1381 = _1379 - _1380;
            highp vec4 _1391 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1380 + _1377) * 0.125, _1354, _1321));
            highp vec4 _1395 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1380 + 1.0) + _1377) * 0.125, _1354, _1321));
            highp vec4 _1397 = (_1391 * (1.0 - _1381)) + (_1395 * _1381);
            highp vec3 _1398 = _1397.xyz;
            highp vec3 _1411;
            switch (0u)
            {
                case 0u:
                {
                    highp float _1401 = _1397.x;
                    if (_1401 == 0.0)
                    {
                        _1411 = vec3(0.0);
                        break;
                    }
                    _1411 = (((_1398 * _1397.w) / vec3(_1401)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1413 = 1.0 + (_1282 * _1282);
            _1423 = vec3(_1318.x ? vec3(0.0).x : _1317.x, _1318.y ? vec3(0.0).y : _1317.y, _1318.z ? vec3(0.0).z : _1317.z);
            _1424 = (_1398 * (0.0596831031143665313720703125 * _1413)) + (_1411 * ((0.01627720706164836883544921875 * _1413) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1282), 1.5)));
            break;
        }
    }
    highp vec3 _1432;
    if (dot(_130, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1432 = _1424 + (_1423 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1432 = _1424;
    }
    highp vec3 _1446 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1432, _1253, vec3(float(_774))), _762, vec3(float(_171)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1448 = vec4(_1446.x, _1446.y, _1446.z, _109.w);
    _1448.w = 1.0;
    highp vec4 _1459;
    if ((u_fragParams.u_camera.z <= 0.0) && (_130.z < 0.0))
    {
        highp vec3 _1457 = pow(_162.xyz, vec3(0.5));
        _1459 = vec4(_1457.x, _1457.y, _1457.z, _1448.w);
    }
    else
    {
        _1459 = _1448;
    }
    out_var_SV_Target0 = _1459;
    out_var_SV_Target1 = _138;
    gl_FragDepth = _143;
}

