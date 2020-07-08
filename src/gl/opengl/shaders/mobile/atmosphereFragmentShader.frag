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

vec4 _107;

void main()
{
    highp float _115 = u_fragParams.u_earthCenter.w + 60000.0;
    highp vec3 _128 = normalize(varying_TEXCOORD1);
    highp vec4 _132 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0);
    highp vec4 _136 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, varying_TEXCOORD0);
    highp vec4 _140 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0);
    highp float _141 = _140.x;
    highp float _146 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp vec2 _164 = _136.zw;
    highp vec3 _169 = vec3(_136.zw, float(int(sign(_136.y))) * sqrt(1.0 - dot(_164, _164)));
    highp vec3 _172 = pow(abs(_132.xyz), vec3(2.2000000476837158203125));
    highp float _178 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _146) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _141 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _146))) * u_cameraPlaneParams.s_CameraFarPlane;
    highp vec3 _180 = u_fragParams.u_camera.xyz + (_128 * _178);
    highp vec3 _181 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    highp float _182 = dot(_181, _128);
    highp float _184 = _182 * _182;
    highp float _186 = -_182;
    highp float _187 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _190 = _186 - sqrt(_187 - (dot(_181, _181) - _184));
    bool _191 = _190 > 0.0;
    highp vec3 _196;
    if (_191)
    {
        _196 = u_fragParams.u_camera.xyz + (_128 * _190);
    }
    else
    {
        _196 = _180;
    }
    highp vec3 _206;
    if (length(_169) == 0.0)
    {
        _206 = normalize(_180 - u_fragParams.u_earthCenter.xyz);
    }
    else
    {
        _206 = _169;
    }
    bool _207 = _141 < 0.75;
    highp vec3 _795;
    if (_207)
    {
        highp vec3 _210 = _180 - u_fragParams.u_earthCenter.xyz;
        highp float _213 = length(_210);
        highp float _215 = dot(_210, u_fragParams.u_sunDirection.xyz) / _213;
        highp float _217 = _115 - u_fragParams.u_earthCenter.w;
        highp vec4 _228 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_215 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_213 - u_fragParams.u_earthCenter.w) / _217) * 0.9375)));
        highp float _235 = u_fragParams.u_earthCenter.w / _213;
        highp float _241 = _115 * _115;
        highp float _243 = sqrt(_241 - _187);
        highp float _244 = _213 * _213;
        highp float _247 = sqrt(max(_244 - _187, 0.0));
        highp float _258 = _115 - _213;
        highp vec4 _271 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_213) * _215) + sqrt(max((_244 * ((_215 * _215) - 1.0)) + _241, 0.0)), 0.0) - _258) / ((_247 + _243) - _258)) * 0.99609375), 0.0078125 + ((_247 / _243) * 0.984375)));
        highp float _288 = max(0.0, min(0.0, _178));
        highp vec3 _291 = normalize(_210 - _181);
        highp float _292 = length(_181);
        highp float _293 = dot(_181, _291);
        highp float _300 = (-_293) - sqrt(((_293 * _293) - (_292 * _292)) + _241);
        bool _301 = _300 > 0.0;
        highp vec3 _307;
        highp float _308;
        if (_301)
        {
            _307 = _181 + (_291 * _300);
            _308 = _293 + _300;
        }
        else
        {
            _307 = _181;
            _308 = _293;
        }
        highp float _327;
        highp float _309 = _301 ? _115 : _292;
        highp float _310 = _308 / _309;
        highp float _311 = dot(_307, u_fragParams.u_sunDirection.xyz);
        highp float _312 = _311 / _309;
        highp float _313 = dot(_291, u_fragParams.u_sunDirection.xyz);
        highp float _315 = length(_210 - _307);
        highp float _317 = _309 * _309;
        highp float _320 = _317 * ((_310 * _310) - 1.0);
        bool _323 = (_310 < 0.0) && ((_320 + _187) >= 0.0);
        highp vec3 _452;
        switch (0u)
        {
            case 0u:
            {
                _327 = (2.0 * _309) * _310;
                highp float _332 = clamp(sqrt((_315 * (_315 + _327)) + _317), u_fragParams.u_earthCenter.w, _115);
                highp float _335 = clamp((_308 + _315) / _332, -1.0, 1.0);
                if (_323)
                {
                    highp float _393 = -_335;
                    highp float _394 = _332 * _332;
                    highp float _397 = sqrt(max(_394 - _187, 0.0));
                    highp float _408 = _115 - _332;
                    highp float _422 = -_310;
                    highp float _425 = sqrt(max(_317 - _187, 0.0));
                    highp float _436 = _115 - _309;
                    _452 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_332) * _393) + sqrt(max((_394 * ((_393 * _393) - 1.0)) + _241, 0.0)), 0.0) - _408) / ((_397 + _243) - _408)) * 0.99609375), 0.0078125 + ((_397 / _243) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_309) * _422) + sqrt(max((_317 * ((_422 * _422) - 1.0)) + _241, 0.0)), 0.0) - _436) / ((_425 + _243) - _436)) * 0.99609375), 0.0078125 + ((_425 / _243) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _341 = sqrt(max(_317 - _187, 0.0));
                    highp float _349 = _115 - _309;
                    highp float _363 = _332 * _332;
                    highp float _366 = sqrt(max(_363 - _187, 0.0));
                    highp float _377 = _115 - _332;
                    _452 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_309) * _310) + sqrt(max(_320 + _241, 0.0)), 0.0) - _349) / ((_341 + _243) - _349)) * 0.99609375), 0.0078125 + ((_341 / _243) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_332) * _335) + sqrt(max((_363 * ((_335 * _335) - 1.0)) + _241, 0.0)), 0.0) - _377) / ((_366 + _243) - _377)) * 0.99609375), 0.0078125 + ((_366 / _243) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _455 = sqrt(max(_317 - _187, 0.0));
        highp float _456 = _455 / _243;
        highp float _458 = 0.015625 + (_456 * 0.96875);
        highp float _461 = ((_308 * _308) - _317) + _187;
        highp float _494;
        if (_323)
        {
            highp float _484 = _309 - u_fragParams.u_earthCenter.w;
            _494 = 0.5 - (0.5 * (0.0078125 + (((_455 == _484) ? 0.0 : ((((-_308) - sqrt(max(_461, 0.0))) - _484) / (_455 - _484))) * 0.984375)));
        }
        else
        {
            highp float _471 = _115 - _309;
            _494 = 0.5 + (0.5 * (0.0078125 + (((((-_308) + sqrt(max(_461 + (_243 * _243), 0.0))) - _471) / ((_455 + _243) - _471)) * 0.984375)));
        }
        highp float _499 = -u_fragParams.u_earthCenter.w;
        highp float _506 = _243 - _217;
        highp float _507 = (max((_499 * _312) + sqrt(max((_187 * ((_312 * _312) - 1.0)) + _241, 0.0)), 0.0) - _217) / _506;
        highp float _509 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _506;
        highp float _516 = 0.015625 + ((max(1.0 - (_507 / _509), 0.0) / (1.0 + _507)) * 0.96875);
        highp float _518 = (_313 + 1.0) * 3.5;
        highp float _519 = floor(_518);
        highp float _520 = _518 - _519;
        highp float _524 = _519 + 1.0;
        highp vec4 _530 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_519 + _516) * 0.125, _494, _458));
        highp float _531 = 1.0 - _520;
        highp vec4 _534 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_524 + _516) * 0.125, _494, _458));
        highp vec4 _536 = (_530 * _531) + (_534 * _520);
        highp vec3 _537 = _536.xyz;
        highp vec3 _550;
        switch (0u)
        {
            case 0u:
            {
                highp float _540 = _536.x;
                if (_540 == 0.0)
                {
                    _550 = vec3(0.0);
                    break;
                }
                _550 = (((_537 * _536.w) / vec3(_540)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _552 = max(_315 - _288, 0.0);
        highp float _557 = clamp(sqrt((_552 * (_552 + _327)) + _317), u_fragParams.u_earthCenter.w, _115);
        highp float _558 = _308 + _552;
        highp float _561 = (_311 + (_552 * _313)) / _557;
        highp float _562 = _557 * _557;
        highp float _565 = sqrt(max(_562 - _187, 0.0));
        highp float _566 = _565 / _243;
        highp float _568 = 0.015625 + (_566 * 0.96875);
        highp float _571 = ((_558 * _558) - _562) + _187;
        highp float _604;
        if (_323)
        {
            highp float _594 = _557 - u_fragParams.u_earthCenter.w;
            _604 = 0.5 - (0.5 * (0.0078125 + (((_565 == _594) ? 0.0 : ((((-_558) - sqrt(max(_571, 0.0))) - _594) / (_565 - _594))) * 0.984375)));
        }
        else
        {
            highp float _581 = _115 - _557;
            _604 = 0.5 + (0.5 * (0.0078125 + (((((-_558) + sqrt(max(_571 + (_243 * _243), 0.0))) - _581) / ((_565 + _243) - _581)) * 0.984375)));
        }
        highp float _615 = (max((_499 * _561) + sqrt(max((_187 * ((_561 * _561) - 1.0)) + _241, 0.0)), 0.0) - _217) / _506;
        highp float _622 = 0.015625 + ((max(1.0 - (_615 / _509), 0.0) / (1.0 + _615)) * 0.96875);
        highp vec4 _630 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_519 + _622) * 0.125, _604, _568));
        highp vec4 _633 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_524 + _622) * 0.125, _604, _568));
        highp vec4 _635 = (_630 * _531) + (_633 * _520);
        highp vec3 _636 = _635.xyz;
        highp vec3 _649;
        switch (0u)
        {
            case 0u:
            {
                highp float _639 = _635.x;
                if (_639 == 0.0)
                {
                    _649 = vec3(0.0);
                    break;
                }
                _649 = (((_636 * _635.w) / vec3(_639)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _756;
        if (_288 > 0.0)
        {
            highp vec3 _755;
            switch (0u)
            {
                case 0u:
                {
                    highp float _656 = clamp(_558 / _557, -1.0, 1.0);
                    if (_323)
                    {
                        highp float _705 = -_656;
                        highp float _716 = _115 - _557;
                        highp float _729 = -_310;
                        highp float _740 = _115 - _309;
                        _755 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_557) * _705) + sqrt(max((_562 * ((_705 * _705) - 1.0)) + _241, 0.0)), 0.0) - _716) / ((_565 + _243) - _716)) * 0.99609375), 0.0078125 + (_566 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_309) * _729) + sqrt(max((_317 * ((_729 * _729) - 1.0)) + _241, 0.0)), 0.0) - _740) / ((_455 + _243) - _740)) * 0.99609375), 0.0078125 + (_456 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _667 = _115 - _309;
                        highp float _690 = _115 - _557;
                        _755 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_309) * _310) + sqrt(max(_320 + _241, 0.0)), 0.0) - _667) / ((_455 + _243) - _667)) * 0.99609375), 0.0078125 + (_456 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_557) * _656) + sqrt(max((_562 * ((_656 * _656) - 1.0)) + _241, 0.0)), 0.0) - _690) / ((_565 + _243) - _690)) * 0.99609375), 0.0078125 + (_566 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _756 = _755;
        }
        else
        {
            _756 = _452;
        }
        highp vec3 _758 = _537 - (_756 * _636);
        highp vec3 _760 = _550 - (_756 * _649);
        highp float _761 = _760.x;
        highp float _762 = _758.x;
        highp vec3 _777;
        switch (0u)
        {
            case 0u:
            {
                if (_762 == 0.0)
                {
                    _777 = vec3(0.0);
                    break;
                }
                _777 = (((vec4(_762, _758.yz, _761).xyz * _761) / vec3(_762)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _781 = 1.0 + (_313 * _313);
        _795 = (((_172.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_271.xyz * smoothstep(_235 * (-0.004674999974668025970458984375), _235 * 0.004674999974668025970458984375, _215 - (-sqrt(max(1.0 - (_235 * _235), 0.0))))) * max(dot(_206, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_228.xyz * (1.0 + (dot(_206, _210) / _213))) * 0.5))) * _452) + (((_758 * (0.0596831031143665313720703125 * _781)) + ((_777 * smoothstep(0.0, 0.00999999977648258209228515625, _312)) * ((0.01627720706164836883544921875 * _781) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _313), 1.5)))) * mix(0.5, 1.0, min(1.0, pow(_141, 6.0) * 6.0)));
    }
    else
    {
        _795 = vec3(0.0);
    }
    highp vec3 _1272;
    if (_191)
    {
        highp vec3 _799 = _196 - u_fragParams.u_earthCenter.xyz;
        highp float _802 = length(_799);
        highp float _804 = dot(_799, u_fragParams.u_sunDirection.xyz) / _802;
        highp float _806 = _115 - u_fragParams.u_earthCenter.w;
        highp vec4 _817 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_804 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_802 - u_fragParams.u_earthCenter.w) / _806) * 0.9375)));
        highp float _824 = u_fragParams.u_earthCenter.w / _802;
        highp float _830 = _115 * _115;
        highp float _832 = sqrt(_830 - _187);
        highp float _833 = _802 * _802;
        highp float _836 = sqrt(max(_833 - _187, 0.0));
        highp float _847 = _115 - _802;
        highp vec4 _860 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_802) * _804) + sqrt(max((_833 * ((_804 * _804) - 1.0)) + _830, 0.0)), 0.0) - _847) / ((_836 + _832) - _847)) * 0.99609375), 0.0078125 + ((_836 / _832) * 0.984375)));
        highp vec3 _878 = normalize(_799 - _181);
        highp float _879 = length(_181);
        highp float _880 = dot(_181, _878);
        highp float _887 = (-_880) - sqrt(((_880 * _880) - (_879 * _879)) + _830);
        bool _888 = _887 > 0.0;
        highp vec3 _894;
        highp float _895;
        if (_888)
        {
            _894 = _181 + (_878 * _887);
            _895 = _880 + _887;
        }
        else
        {
            _894 = _181;
            _895 = _880;
        }
        highp float _914;
        highp float _896 = _888 ? _115 : _879;
        highp float _897 = _895 / _896;
        highp float _898 = dot(_894, u_fragParams.u_sunDirection.xyz);
        highp float _899 = _898 / _896;
        highp float _900 = dot(_878, u_fragParams.u_sunDirection.xyz);
        highp float _902 = length(_799 - _894);
        highp float _904 = _896 * _896;
        highp float _907 = _904 * ((_897 * _897) - 1.0);
        bool _910 = (_897 < 0.0) && ((_907 + _187) >= 0.0);
        highp vec3 _1039;
        switch (0u)
        {
            case 0u:
            {
                _914 = (2.0 * _896) * _897;
                highp float _919 = clamp(sqrt((_902 * (_902 + _914)) + _904), u_fragParams.u_earthCenter.w, _115);
                highp float _922 = clamp((_895 + _902) / _919, -1.0, 1.0);
                if (_910)
                {
                    highp float _980 = -_922;
                    highp float _981 = _919 * _919;
                    highp float _984 = sqrt(max(_981 - _187, 0.0));
                    highp float _995 = _115 - _919;
                    highp float _1009 = -_897;
                    highp float _1012 = sqrt(max(_904 - _187, 0.0));
                    highp float _1023 = _115 - _896;
                    _1039 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_919) * _980) + sqrt(max((_981 * ((_980 * _980) - 1.0)) + _830, 0.0)), 0.0) - _995) / ((_984 + _832) - _995)) * 0.99609375), 0.0078125 + ((_984 / _832) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_896) * _1009) + sqrt(max((_904 * ((_1009 * _1009) - 1.0)) + _830, 0.0)), 0.0) - _1023) / ((_1012 + _832) - _1023)) * 0.99609375), 0.0078125 + ((_1012 / _832) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _928 = sqrt(max(_904 - _187, 0.0));
                    highp float _936 = _115 - _896;
                    highp float _950 = _919 * _919;
                    highp float _953 = sqrt(max(_950 - _187, 0.0));
                    highp float _964 = _115 - _919;
                    _1039 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_896) * _897) + sqrt(max(_907 + _830, 0.0)), 0.0) - _936) / ((_928 + _832) - _936)) * 0.99609375), 0.0078125 + ((_928 / _832) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_919) * _922) + sqrt(max((_950 * ((_922 * _922) - 1.0)) + _830, 0.0)), 0.0) - _964) / ((_953 + _832) - _964)) * 0.99609375), 0.0078125 + ((_953 / _832) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _1042 = sqrt(max(_904 - _187, 0.0));
        highp float _1045 = 0.015625 + ((_1042 / _832) * 0.96875);
        highp float _1048 = ((_895 * _895) - _904) + _187;
        highp float _1081;
        if (_910)
        {
            highp float _1071 = _896 - u_fragParams.u_earthCenter.w;
            _1081 = 0.5 - (0.5 * (0.0078125 + (((_1042 == _1071) ? 0.0 : ((((-_895) - sqrt(max(_1048, 0.0))) - _1071) / (_1042 - _1071))) * 0.984375)));
        }
        else
        {
            highp float _1058 = _115 - _896;
            _1081 = 0.5 + (0.5 * (0.0078125 + (((((-_895) + sqrt(max(_1048 + (_832 * _832), 0.0))) - _1058) / ((_1042 + _832) - _1058)) * 0.984375)));
        }
        highp float _1086 = -u_fragParams.u_earthCenter.w;
        highp float _1093 = _832 - _806;
        highp float _1094 = (max((_1086 * _899) + sqrt(max((_187 * ((_899 * _899) - 1.0)) + _830, 0.0)), 0.0) - _806) / _1093;
        highp float _1096 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1093;
        highp float _1103 = 0.015625 + ((max(1.0 - (_1094 / _1096), 0.0) / (1.0 + _1094)) * 0.96875);
        highp float _1105 = (_900 + 1.0) * 3.5;
        highp float _1106 = floor(_1105);
        highp float _1107 = _1105 - _1106;
        highp float _1111 = _1106 + 1.0;
        highp vec4 _1117 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1106 + _1103) * 0.125, _1081, _1045));
        highp float _1118 = 1.0 - _1107;
        highp vec4 _1121 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1111 + _1103) * 0.125, _1081, _1045));
        highp vec4 _1123 = (_1117 * _1118) + (_1121 * _1107);
        highp vec3 _1124 = _1123.xyz;
        highp vec3 _1137;
        switch (0u)
        {
            case 0u:
            {
                highp float _1127 = _1123.x;
                if (_1127 == 0.0)
                {
                    _1137 = vec3(0.0);
                    break;
                }
                _1137 = (((_1124 * _1123.w) / vec3(_1127)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1138 = max(_902, 0.0);
        highp float _1143 = clamp(sqrt((_1138 * (_1138 + _914)) + _904), u_fragParams.u_earthCenter.w, _115);
        highp float _1144 = _895 + _1138;
        highp float _1147 = (_898 + (_1138 * _900)) / _1143;
        highp float _1148 = _1143 * _1143;
        highp float _1151 = sqrt(max(_1148 - _187, 0.0));
        highp float _1154 = 0.015625 + ((_1151 / _832) * 0.96875);
        highp float _1157 = ((_1144 * _1144) - _1148) + _187;
        highp float _1190;
        if (_910)
        {
            highp float _1180 = _1143 - u_fragParams.u_earthCenter.w;
            _1190 = 0.5 - (0.5 * (0.0078125 + (((_1151 == _1180) ? 0.0 : ((((-_1144) - sqrt(max(_1157, 0.0))) - _1180) / (_1151 - _1180))) * 0.984375)));
        }
        else
        {
            highp float _1167 = _115 - _1143;
            _1190 = 0.5 + (0.5 * (0.0078125 + (((((-_1144) + sqrt(max(_1157 + (_832 * _832), 0.0))) - _1167) / ((_1151 + _832) - _1167)) * 0.984375)));
        }
        highp float _1201 = (max((_1086 * _1147) + sqrt(max((_187 * ((_1147 * _1147) - 1.0)) + _830, 0.0)), 0.0) - _806) / _1093;
        highp float _1208 = 0.015625 + ((max(1.0 - (_1201 / _1096), 0.0) / (1.0 + _1201)) * 0.96875);
        highp vec4 _1216 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1106 + _1208) * 0.125, _1190, _1154));
        highp vec4 _1219 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1111 + _1208) * 0.125, _1190, _1154));
        highp vec4 _1221 = (_1216 * _1118) + (_1219 * _1107);
        highp vec3 _1222 = _1221.xyz;
        highp vec3 _1235;
        switch (0u)
        {
            case 0u:
            {
                highp float _1225 = _1221.x;
                if (_1225 == 0.0)
                {
                    _1235 = vec3(0.0);
                    break;
                }
                _1235 = (((_1222 * _1221.w) / vec3(_1225)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _1237 = _1124 - (_1039 * _1222);
        highp vec3 _1239 = _1137 - (_1039 * _1235);
        highp float _1240 = _1239.x;
        highp float _1241 = _1237.x;
        highp vec3 _1256;
        switch (0u)
        {
            case 0u:
            {
                if (_1241 == 0.0)
                {
                    _1256 = vec3(0.0);
                    break;
                }
                _1256 = (((vec4(_1241, _1237.yz, _1240).xyz * _1240) / vec3(_1241)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1260 = 1.0 + (_900 * _900);
        _1272 = (((_172.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_860.xyz * smoothstep(_824 * (-0.004674999974668025970458984375), _824 * 0.004674999974668025970458984375, _804 - (-sqrt(max(1.0 - (_824 * _824), 0.0))))) * max(dot(_206, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_817.xyz * (1.0 + (dot(_206, _799) / _802))) * 0.5))) * _1039) + ((_1237 * (0.0596831031143665313720703125 * _1260)) + ((_1256 * smoothstep(0.0, 0.00999999977648258209228515625, _899)) * ((0.01627720706164836883544921875 * _1260) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _900), 1.5))));
    }
    else
    {
        _1272 = vec3(0.0);
    }
    highp vec3 _1442;
    highp vec3 _1443;
    switch (0u)
    {
        case 0u:
        {
            highp float _1278 = length(_181);
            highp float _1281 = _115 * _115;
            highp float _1284 = _186 - sqrt((_184 - (_1278 * _1278)) + _1281);
            bool _1285 = _1284 > 0.0;
            highp vec3 _1295;
            highp float _1296;
            if (_1285)
            {
                _1295 = _181 + (_128 * _1284);
                _1296 = _182 + _1284;
            }
            else
            {
                if (_1278 > _115)
                {
                    _1442 = vec3(1.0);
                    _1443 = vec3(0.0);
                    break;
                }
                _1295 = _181;
                _1296 = _182;
            }
            highp float _1297 = _1285 ? _115 : _1278;
            highp float _1298 = _1296 / _1297;
            highp float _1300 = dot(_1295, u_fragParams.u_sunDirection.xyz) / _1297;
            highp float _1301 = dot(_128, u_fragParams.u_sunDirection.xyz);
            highp float _1303 = _1297 * _1297;
            highp float _1306 = _1303 * ((_1298 * _1298) - 1.0);
            bool _1309 = (_1298 < 0.0) && ((_1306 + _187) >= 0.0);
            highp float _1311 = sqrt(_1281 - _187);
            highp float _1314 = sqrt(max(_1303 - _187, 0.0));
            highp float _1322 = _115 - _1297;
            highp float _1325 = (_1314 + _1311) - _1322;
            highp float _1327 = _1314 / _1311;
            highp vec4 _1335 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1297) * _1298) + sqrt(max(_1306 + _1281, 0.0)), 0.0) - _1322) / _1325) * 0.99609375), 0.0078125 + (_1327 * 0.984375)));
            highp vec3 _1336 = _1335.xyz;
            bvec3 _1337 = bvec3(_1309);
            highp float _1340 = 0.015625 + (_1327 * 0.96875);
            highp float _1343 = ((_1296 * _1296) - _1303) + _187;
            highp float _1373;
            if (_1309)
            {
                highp float _1363 = _1297 - u_fragParams.u_earthCenter.w;
                _1373 = 0.5 - (0.5 * (0.0078125 + (((_1314 == _1363) ? 0.0 : ((((-_1296) - sqrt(max(_1343, 0.0))) - _1363) / (_1314 - _1363))) * 0.984375)));
            }
            else
            {
                _1373 = 0.5 + (0.5 * (0.0078125 + (((((-_1296) + sqrt(max(_1343 + (_1311 * _1311), 0.0))) - _1322) / _1325) * 0.984375)));
            }
            highp float _1384 = _115 - u_fragParams.u_earthCenter.w;
            highp float _1386 = _1311 - _1384;
            highp float _1387 = (max(((-u_fragParams.u_earthCenter.w) * _1300) + sqrt(max((_187 * ((_1300 * _1300) - 1.0)) + _1281, 0.0)), 0.0) - _1384) / _1386;
            highp float _1396 = 0.015625 + ((max(1.0 - (_1387 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1386)), 0.0) / (1.0 + _1387)) * 0.96875);
            highp float _1398 = (_1301 + 1.0) * 3.5;
            highp float _1399 = floor(_1398);
            highp float _1400 = _1398 - _1399;
            highp vec4 _1410 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1399 + _1396) * 0.125, _1373, _1340));
            highp vec4 _1414 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1399 + 1.0) + _1396) * 0.125, _1373, _1340));
            highp vec4 _1416 = (_1410 * (1.0 - _1400)) + (_1414 * _1400);
            highp vec3 _1417 = _1416.xyz;
            highp vec3 _1430;
            switch (0u)
            {
                case 0u:
                {
                    highp float _1420 = _1416.x;
                    if (_1420 == 0.0)
                    {
                        _1430 = vec3(0.0);
                        break;
                    }
                    _1430 = (((_1417 * _1416.w) / vec3(_1420)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1432 = 1.0 + (_1301 * _1301);
            _1442 = vec3(_1337.x ? vec3(0.0).x : _1336.x, _1337.y ? vec3(0.0).y : _1336.y, _1337.z ? vec3(0.0).z : _1336.z);
            _1443 = (_1417 * (0.0596831031143665313720703125 * _1432)) + (_1430 * ((0.01627720706164836883544921875 * _1432) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1301), 1.5)));
            break;
        }
    }
    highp vec3 _1451;
    if (dot(_128, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1451 = _1443 + (_1442 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1451 = _1443;
    }
    highp vec3 _1469 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1451, _1272, vec3(float(_191))), _795, vec3(float(_207) * min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _141))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1471 = vec4(_1469.x, _1469.y, _1469.z, _107.w);
    _1471.w = 1.0;
    out_var_SV_Target0 = _1471;
    out_var_SV_Target1 = _136;
    gl_FragDepth = _141;
}

