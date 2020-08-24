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
    highp float _186 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _187 = _186 - (dot(_181, _181) - _184);
    highp float _194;
    if (_187 > 0.0)
    {
        _194 = (-_182) - sqrt(_187);
    }
    else
    {
        _194 = 0.0;
    }
    bool _195 = _194 > 0.0;
    highp vec3 _200;
    if (_195)
    {
        _200 = u_fragParams.u_camera.xyz + (_128 * _194);
    }
    else
    {
        _200 = _180;
    }
    highp vec3 _210;
    if (length(_169) == 0.0)
    {
        _210 = normalize(_180 - u_fragParams.u_earthCenter.xyz);
    }
    else
    {
        _210 = _169;
    }
    bool _211 = _141 < 0.75;
    highp vec3 _800;
    if (_211)
    {
        highp vec3 _214 = _180 - u_fragParams.u_earthCenter.xyz;
        highp float _217 = length(_214);
        highp float _219 = dot(_214, u_fragParams.u_sunDirection.xyz) / _217;
        highp float _221 = _115 - u_fragParams.u_earthCenter.w;
        highp vec4 _232 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_219 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_217 - u_fragParams.u_earthCenter.w) / _221) * 0.9375)));
        highp float _239 = u_fragParams.u_earthCenter.w / _217;
        highp float _245 = _115 * _115;
        highp float _247 = sqrt(_245 - _186);
        highp float _248 = _217 * _217;
        highp float _251 = sqrt(max(_248 - _186, 0.0));
        highp float _262 = _115 - _217;
        highp vec4 _275 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_217) * _219) + sqrt(max((_248 * ((_219 * _219) - 1.0)) + _245, 0.0)), 0.0) - _262) / ((_251 + _247) - _262)) * 0.99609375), 0.0078125 + ((_251 / _247) * 0.984375)));
        highp float _292 = max(0.0, min(0.0, _178));
        highp vec3 _295 = normalize(_214 - _181);
        highp float _296 = length(_181);
        highp float _297 = dot(_181, _295);
        highp float _305 = (-_297) - sqrt(max(((_297 * _297) - (_296 * _296)) + _245, 0.0));
        bool _306 = _305 > 0.0;
        highp vec3 _312;
        highp float _313;
        if (_306)
        {
            _312 = _181 + (_295 * _305);
            _313 = _297 + _305;
        }
        else
        {
            _312 = _181;
            _313 = _297;
        }
        highp float _332;
        highp float _314 = _306 ? _115 : _296;
        highp float _315 = _313 / _314;
        highp float _316 = dot(_312, u_fragParams.u_sunDirection.xyz);
        highp float _317 = _316 / _314;
        highp float _318 = dot(_295, u_fragParams.u_sunDirection.xyz);
        highp float _320 = length(_214 - _312);
        highp float _322 = _314 * _314;
        highp float _325 = _322 * ((_315 * _315) - 1.0);
        bool _328 = (_315 < 0.0) && ((_325 + _186) >= 0.0);
        highp vec3 _457;
        switch (0u)
        {
            case 0u:
            {
                _332 = (2.0 * _314) * _315;
                highp float _337 = clamp(sqrt((_320 * (_320 + _332)) + _322), u_fragParams.u_earthCenter.w, _115);
                highp float _340 = clamp((_313 + _320) / _337, -1.0, 1.0);
                if (_328)
                {
                    highp float _398 = -_340;
                    highp float _399 = _337 * _337;
                    highp float _402 = sqrt(max(_399 - _186, 0.0));
                    highp float _413 = _115 - _337;
                    highp float _427 = -_315;
                    highp float _430 = sqrt(max(_322 - _186, 0.0));
                    highp float _441 = _115 - _314;
                    _457 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_337) * _398) + sqrt(max((_399 * ((_398 * _398) - 1.0)) + _245, 0.0)), 0.0) - _413) / ((_402 + _247) - _413)) * 0.99609375), 0.0078125 + ((_402 / _247) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_314) * _427) + sqrt(max((_322 * ((_427 * _427) - 1.0)) + _245, 0.0)), 0.0) - _441) / ((_430 + _247) - _441)) * 0.99609375), 0.0078125 + ((_430 / _247) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _346 = sqrt(max(_322 - _186, 0.0));
                    highp float _354 = _115 - _314;
                    highp float _368 = _337 * _337;
                    highp float _371 = sqrt(max(_368 - _186, 0.0));
                    highp float _382 = _115 - _337;
                    _457 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_314) * _315) + sqrt(max(_325 + _245, 0.0)), 0.0) - _354) / ((_346 + _247) - _354)) * 0.99609375), 0.0078125 + ((_346 / _247) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_337) * _340) + sqrt(max((_368 * ((_340 * _340) - 1.0)) + _245, 0.0)), 0.0) - _382) / ((_371 + _247) - _382)) * 0.99609375), 0.0078125 + ((_371 / _247) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _460 = sqrt(max(_322 - _186, 0.0));
        highp float _461 = _460 / _247;
        highp float _463 = 0.015625 + (_461 * 0.96875);
        highp float _466 = ((_313 * _313) - _322) + _186;
        highp float _499;
        if (_328)
        {
            highp float _489 = _314 - u_fragParams.u_earthCenter.w;
            _499 = 0.5 - (0.5 * (0.0078125 + (((_460 == _489) ? 0.0 : ((((-_313) - sqrt(max(_466, 0.0))) - _489) / (_460 - _489))) * 0.984375)));
        }
        else
        {
            highp float _476 = _115 - _314;
            _499 = 0.5 + (0.5 * (0.0078125 + (((((-_313) + sqrt(max(_466 + (_247 * _247), 0.0))) - _476) / ((_460 + _247) - _476)) * 0.984375)));
        }
        highp float _504 = -u_fragParams.u_earthCenter.w;
        highp float _511 = _247 - _221;
        highp float _512 = (max((_504 * _317) + sqrt(max((_186 * ((_317 * _317) - 1.0)) + _245, 0.0)), 0.0) - _221) / _511;
        highp float _514 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _511;
        highp float _521 = 0.015625 + ((max(1.0 - (_512 / _514), 0.0) / (1.0 + _512)) * 0.96875);
        highp float _523 = (_318 + 1.0) * 3.5;
        highp float _524 = floor(_523);
        highp float _525 = _523 - _524;
        highp float _529 = _524 + 1.0;
        highp vec4 _535 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_524 + _521) * 0.125, _499, _463));
        highp float _536 = 1.0 - _525;
        highp vec4 _539 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_529 + _521) * 0.125, _499, _463));
        highp vec4 _541 = (_535 * _536) + (_539 * _525);
        highp vec3 _542 = _541.xyz;
        highp vec3 _555;
        switch (0u)
        {
            case 0u:
            {
                highp float _545 = _541.x;
                if (_545 == 0.0)
                {
                    _555 = vec3(0.0);
                    break;
                }
                _555 = (((_542 * _541.w) / vec3(_545)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _557 = max(_320 - _292, 0.0);
        highp float _562 = clamp(sqrt((_557 * (_557 + _332)) + _322), u_fragParams.u_earthCenter.w, _115);
        highp float _563 = _313 + _557;
        highp float _566 = (_316 + (_557 * _318)) / _562;
        highp float _567 = _562 * _562;
        highp float _570 = sqrt(max(_567 - _186, 0.0));
        highp float _571 = _570 / _247;
        highp float _573 = 0.015625 + (_571 * 0.96875);
        highp float _576 = ((_563 * _563) - _567) + _186;
        highp float _609;
        if (_328)
        {
            highp float _599 = _562 - u_fragParams.u_earthCenter.w;
            _609 = 0.5 - (0.5 * (0.0078125 + (((_570 == _599) ? 0.0 : ((((-_563) - sqrt(max(_576, 0.0))) - _599) / (_570 - _599))) * 0.984375)));
        }
        else
        {
            highp float _586 = _115 - _562;
            _609 = 0.5 + (0.5 * (0.0078125 + (((((-_563) + sqrt(max(_576 + (_247 * _247), 0.0))) - _586) / ((_570 + _247) - _586)) * 0.984375)));
        }
        highp float _620 = (max((_504 * _566) + sqrt(max((_186 * ((_566 * _566) - 1.0)) + _245, 0.0)), 0.0) - _221) / _511;
        highp float _627 = 0.015625 + ((max(1.0 - (_620 / _514), 0.0) / (1.0 + _620)) * 0.96875);
        highp vec4 _635 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_524 + _627) * 0.125, _609, _573));
        highp vec4 _638 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_529 + _627) * 0.125, _609, _573));
        highp vec4 _640 = (_635 * _536) + (_638 * _525);
        highp vec3 _641 = _640.xyz;
        highp vec3 _654;
        switch (0u)
        {
            case 0u:
            {
                highp float _644 = _640.x;
                if (_644 == 0.0)
                {
                    _654 = vec3(0.0);
                    break;
                }
                _654 = (((_641 * _640.w) / vec3(_644)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _761;
        if (_292 > 0.0)
        {
            highp vec3 _760;
            switch (0u)
            {
                case 0u:
                {
                    highp float _661 = clamp(_563 / _562, -1.0, 1.0);
                    if (_328)
                    {
                        highp float _710 = -_661;
                        highp float _721 = _115 - _562;
                        highp float _734 = -_315;
                        highp float _745 = _115 - _314;
                        _760 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_562) * _710) + sqrt(max((_567 * ((_710 * _710) - 1.0)) + _245, 0.0)), 0.0) - _721) / ((_570 + _247) - _721)) * 0.99609375), 0.0078125 + (_571 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_314) * _734) + sqrt(max((_322 * ((_734 * _734) - 1.0)) + _245, 0.0)), 0.0) - _745) / ((_460 + _247) - _745)) * 0.99609375), 0.0078125 + (_461 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _672 = _115 - _314;
                        highp float _695 = _115 - _562;
                        _760 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_314) * _315) + sqrt(max(_325 + _245, 0.0)), 0.0) - _672) / ((_460 + _247) - _672)) * 0.99609375), 0.0078125 + (_461 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_562) * _661) + sqrt(max((_567 * ((_661 * _661) - 1.0)) + _245, 0.0)), 0.0) - _695) / ((_570 + _247) - _695)) * 0.99609375), 0.0078125 + (_571 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _761 = _760;
        }
        else
        {
            _761 = _457;
        }
        highp vec3 _763 = _542 - (_761 * _641);
        highp vec3 _765 = _555 - (_761 * _654);
        highp float _766 = _765.x;
        highp float _767 = _763.x;
        highp vec3 _782;
        switch (0u)
        {
            case 0u:
            {
                if (_767 == 0.0)
                {
                    _782 = vec3(0.0);
                    break;
                }
                _782 = (((vec4(_767, _763.yz, _766).xyz * _766) / vec3(_767)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _786 = 1.0 + (_318 * _318);
        _800 = (((_172.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_275.xyz * smoothstep(_239 * (-0.004674999974668025970458984375), _239 * 0.004674999974668025970458984375, _219 - (-sqrt(max(1.0 - (_239 * _239), 0.0))))) * max(dot(_210, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_232.xyz * (1.0 + (dot(_210, _214) / _217))) * 0.5))) * _457) + (((_763 * (0.0596831031143665313720703125 * _786)) + ((_782 * smoothstep(0.0, 0.00999999977648258209228515625, _317)) * ((0.01627720706164836883544921875 * _786) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _318), 1.5)))) * mix(0.5, 1.0, min(1.0, pow(_141, 6.0) * 6.0)));
    }
    else
    {
        _800 = vec3(0.0);
    }
    highp vec3 _1278;
    if (_195)
    {
        highp vec3 _804 = _200 - u_fragParams.u_earthCenter.xyz;
        highp float _807 = length(_804);
        highp float _809 = dot(_804, u_fragParams.u_sunDirection.xyz) / _807;
        highp float _811 = _115 - u_fragParams.u_earthCenter.w;
        highp vec4 _822 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_809 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_807 - u_fragParams.u_earthCenter.w) / _811) * 0.9375)));
        highp float _829 = u_fragParams.u_earthCenter.w / _807;
        highp float _835 = _115 * _115;
        highp float _837 = sqrt(_835 - _186);
        highp float _838 = _807 * _807;
        highp float _841 = sqrt(max(_838 - _186, 0.0));
        highp float _852 = _115 - _807;
        highp vec4 _865 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_807) * _809) + sqrt(max((_838 * ((_809 * _809) - 1.0)) + _835, 0.0)), 0.0) - _852) / ((_841 + _837) - _852)) * 0.99609375), 0.0078125 + ((_841 / _837) * 0.984375)));
        highp vec3 _883 = normalize(_804 - _181);
        highp float _884 = length(_181);
        highp float _885 = dot(_181, _883);
        highp float _893 = (-_885) - sqrt(max(((_885 * _885) - (_884 * _884)) + _835, 0.0));
        bool _894 = _893 > 0.0;
        highp vec3 _900;
        highp float _901;
        if (_894)
        {
            _900 = _181 + (_883 * _893);
            _901 = _885 + _893;
        }
        else
        {
            _900 = _181;
            _901 = _885;
        }
        highp float _920;
        highp float _902 = _894 ? _115 : _884;
        highp float _903 = _901 / _902;
        highp float _904 = dot(_900, u_fragParams.u_sunDirection.xyz);
        highp float _905 = _904 / _902;
        highp float _906 = dot(_883, u_fragParams.u_sunDirection.xyz);
        highp float _908 = length(_804 - _900);
        highp float _910 = _902 * _902;
        highp float _913 = _910 * ((_903 * _903) - 1.0);
        bool _916 = (_903 < 0.0) && ((_913 + _186) >= 0.0);
        highp vec3 _1045;
        switch (0u)
        {
            case 0u:
            {
                _920 = (2.0 * _902) * _903;
                highp float _925 = clamp(sqrt((_908 * (_908 + _920)) + _910), u_fragParams.u_earthCenter.w, _115);
                highp float _928 = clamp((_901 + _908) / _925, -1.0, 1.0);
                if (_916)
                {
                    highp float _986 = -_928;
                    highp float _987 = _925 * _925;
                    highp float _990 = sqrt(max(_987 - _186, 0.0));
                    highp float _1001 = _115 - _925;
                    highp float _1015 = -_903;
                    highp float _1018 = sqrt(max(_910 - _186, 0.0));
                    highp float _1029 = _115 - _902;
                    _1045 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_925) * _986) + sqrt(max((_987 * ((_986 * _986) - 1.0)) + _835, 0.0)), 0.0) - _1001) / ((_990 + _837) - _1001)) * 0.99609375), 0.0078125 + ((_990 / _837) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_902) * _1015) + sqrt(max((_910 * ((_1015 * _1015) - 1.0)) + _835, 0.0)), 0.0) - _1029) / ((_1018 + _837) - _1029)) * 0.99609375), 0.0078125 + ((_1018 / _837) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _934 = sqrt(max(_910 - _186, 0.0));
                    highp float _942 = _115 - _902;
                    highp float _956 = _925 * _925;
                    highp float _959 = sqrt(max(_956 - _186, 0.0));
                    highp float _970 = _115 - _925;
                    _1045 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_902) * _903) + sqrt(max(_913 + _835, 0.0)), 0.0) - _942) / ((_934 + _837) - _942)) * 0.99609375), 0.0078125 + ((_934 / _837) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_925) * _928) + sqrt(max((_956 * ((_928 * _928) - 1.0)) + _835, 0.0)), 0.0) - _970) / ((_959 + _837) - _970)) * 0.99609375), 0.0078125 + ((_959 / _837) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _1048 = sqrt(max(_910 - _186, 0.0));
        highp float _1051 = 0.015625 + ((_1048 / _837) * 0.96875);
        highp float _1054 = ((_901 * _901) - _910) + _186;
        highp float _1087;
        if (_916)
        {
            highp float _1077 = _902 - u_fragParams.u_earthCenter.w;
            _1087 = 0.5 - (0.5 * (0.0078125 + (((_1048 == _1077) ? 0.0 : ((((-_901) - sqrt(max(_1054, 0.0))) - _1077) / (_1048 - _1077))) * 0.984375)));
        }
        else
        {
            highp float _1064 = _115 - _902;
            _1087 = 0.5 + (0.5 * (0.0078125 + (((((-_901) + sqrt(max(_1054 + (_837 * _837), 0.0))) - _1064) / ((_1048 + _837) - _1064)) * 0.984375)));
        }
        highp float _1092 = -u_fragParams.u_earthCenter.w;
        highp float _1099 = _837 - _811;
        highp float _1100 = (max((_1092 * _905) + sqrt(max((_186 * ((_905 * _905) - 1.0)) + _835, 0.0)), 0.0) - _811) / _1099;
        highp float _1102 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1099;
        highp float _1109 = 0.015625 + ((max(1.0 - (_1100 / _1102), 0.0) / (1.0 + _1100)) * 0.96875);
        highp float _1111 = (_906 + 1.0) * 3.5;
        highp float _1112 = floor(_1111);
        highp float _1113 = _1111 - _1112;
        highp float _1117 = _1112 + 1.0;
        highp vec4 _1123 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1112 + _1109) * 0.125, _1087, _1051));
        highp float _1124 = 1.0 - _1113;
        highp vec4 _1127 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1117 + _1109) * 0.125, _1087, _1051));
        highp vec4 _1129 = (_1123 * _1124) + (_1127 * _1113);
        highp vec3 _1130 = _1129.xyz;
        highp vec3 _1143;
        switch (0u)
        {
            case 0u:
            {
                highp float _1133 = _1129.x;
                if (_1133 == 0.0)
                {
                    _1143 = vec3(0.0);
                    break;
                }
                _1143 = (((_1130 * _1129.w) / vec3(_1133)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1144 = max(_908, 0.0);
        highp float _1149 = clamp(sqrt((_1144 * (_1144 + _920)) + _910), u_fragParams.u_earthCenter.w, _115);
        highp float _1150 = _901 + _1144;
        highp float _1153 = (_904 + (_1144 * _906)) / _1149;
        highp float _1154 = _1149 * _1149;
        highp float _1157 = sqrt(max(_1154 - _186, 0.0));
        highp float _1160 = 0.015625 + ((_1157 / _837) * 0.96875);
        highp float _1163 = ((_1150 * _1150) - _1154) + _186;
        highp float _1196;
        if (_916)
        {
            highp float _1186 = _1149 - u_fragParams.u_earthCenter.w;
            _1196 = 0.5 - (0.5 * (0.0078125 + (((_1157 == _1186) ? 0.0 : ((((-_1150) - sqrt(max(_1163, 0.0))) - _1186) / (_1157 - _1186))) * 0.984375)));
        }
        else
        {
            highp float _1173 = _115 - _1149;
            _1196 = 0.5 + (0.5 * (0.0078125 + (((((-_1150) + sqrt(max(_1163 + (_837 * _837), 0.0))) - _1173) / ((_1157 + _837) - _1173)) * 0.984375)));
        }
        highp float _1207 = (max((_1092 * _1153) + sqrt(max((_186 * ((_1153 * _1153) - 1.0)) + _835, 0.0)), 0.0) - _811) / _1099;
        highp float _1214 = 0.015625 + ((max(1.0 - (_1207 / _1102), 0.0) / (1.0 + _1207)) * 0.96875);
        highp vec4 _1222 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1112 + _1214) * 0.125, _1196, _1160));
        highp vec4 _1225 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1117 + _1214) * 0.125, _1196, _1160));
        highp vec4 _1227 = (_1222 * _1124) + (_1225 * _1113);
        highp vec3 _1228 = _1227.xyz;
        highp vec3 _1241;
        switch (0u)
        {
            case 0u:
            {
                highp float _1231 = _1227.x;
                if (_1231 == 0.0)
                {
                    _1241 = vec3(0.0);
                    break;
                }
                _1241 = (((_1228 * _1227.w) / vec3(_1231)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _1243 = _1130 - (_1045 * _1228);
        highp vec3 _1245 = _1143 - (_1045 * _1241);
        highp float _1246 = _1245.x;
        highp float _1247 = _1243.x;
        highp vec3 _1262;
        switch (0u)
        {
            case 0u:
            {
                if (_1247 == 0.0)
                {
                    _1262 = vec3(0.0);
                    break;
                }
                _1262 = (((vec4(_1247, _1243.yz, _1246).xyz * _1246) / vec3(_1247)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1266 = 1.0 + (_906 * _906);
        _1278 = (((_172.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_865.xyz * smoothstep(_829 * (-0.004674999974668025970458984375), _829 * 0.004674999974668025970458984375, _809 - (-sqrt(max(1.0 - (_829 * _829), 0.0))))) * max(dot(_210, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_822.xyz * (1.0 + (dot(_210, _804) / _807))) * 0.5))) * _1045) + ((_1243 * (0.0596831031143665313720703125 * _1266)) + ((_1262 * smoothstep(0.0, 0.00999999977648258209228515625, _905)) * ((0.01627720706164836883544921875 * _1266) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _906), 1.5))));
    }
    else
    {
        _1278 = vec3(0.0);
    }
    highp vec3 _1450;
    highp vec3 _1451;
    switch (0u)
    {
        case 0u:
        {
            highp float _1284 = length(_181);
            highp float _1288 = _115 * _115;
            highp float _1292 = (-_182) - sqrt(max((_184 - (_1284 * _1284)) + _1288, 0.0));
            bool _1293 = _1292 > 0.0;
            highp vec3 _1303;
            highp float _1304;
            if (_1293)
            {
                _1303 = _181 + (_128 * _1292);
                _1304 = _182 + _1292;
            }
            else
            {
                if (_1284 > _115)
                {
                    _1450 = vec3(1.0);
                    _1451 = vec3(0.0);
                    break;
                }
                _1303 = _181;
                _1304 = _182;
            }
            highp float _1305 = _1293 ? _115 : _1284;
            highp float _1306 = _1304 / _1305;
            highp float _1308 = dot(_1303, u_fragParams.u_sunDirection.xyz) / _1305;
            highp float _1309 = dot(_128, u_fragParams.u_sunDirection.xyz);
            highp float _1311 = _1305 * _1305;
            highp float _1314 = _1311 * ((_1306 * _1306) - 1.0);
            bool _1317 = (_1306 < 0.0) && ((_1314 + _186) >= 0.0);
            highp float _1319 = sqrt(_1288 - _186);
            highp float _1322 = sqrt(max(_1311 - _186, 0.0));
            highp float _1330 = _115 - _1305;
            highp float _1333 = (_1322 + _1319) - _1330;
            highp float _1335 = _1322 / _1319;
            highp vec4 _1343 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1305) * _1306) + sqrt(max(_1314 + _1288, 0.0)), 0.0) - _1330) / _1333) * 0.99609375), 0.0078125 + (_1335 * 0.984375)));
            highp vec3 _1344 = _1343.xyz;
            bvec3 _1345 = bvec3(_1317);
            highp float _1348 = 0.015625 + (_1335 * 0.96875);
            highp float _1351 = ((_1304 * _1304) - _1311) + _186;
            highp float _1381;
            if (_1317)
            {
                highp float _1371 = _1305 - u_fragParams.u_earthCenter.w;
                _1381 = 0.5 - (0.5 * (0.0078125 + (((_1322 == _1371) ? 0.0 : ((((-_1304) - sqrt(max(_1351, 0.0))) - _1371) / (_1322 - _1371))) * 0.984375)));
            }
            else
            {
                _1381 = 0.5 + (0.5 * (0.0078125 + (((((-_1304) + sqrt(max(_1351 + (_1319 * _1319), 0.0))) - _1330) / _1333) * 0.984375)));
            }
            highp float _1392 = _115 - u_fragParams.u_earthCenter.w;
            highp float _1394 = _1319 - _1392;
            highp float _1395 = (max(((-u_fragParams.u_earthCenter.w) * _1308) + sqrt(max((_186 * ((_1308 * _1308) - 1.0)) + _1288, 0.0)), 0.0) - _1392) / _1394;
            highp float _1404 = 0.015625 + ((max(1.0 - (_1395 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1394)), 0.0) / (1.0 + _1395)) * 0.96875);
            highp float _1406 = (_1309 + 1.0) * 3.5;
            highp float _1407 = floor(_1406);
            highp float _1408 = _1406 - _1407;
            highp vec4 _1418 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1407 + _1404) * 0.125, _1381, _1348));
            highp vec4 _1422 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1407 + 1.0) + _1404) * 0.125, _1381, _1348));
            highp vec4 _1424 = (_1418 * (1.0 - _1408)) + (_1422 * _1408);
            highp vec3 _1425 = _1424.xyz;
            highp vec3 _1438;
            switch (0u)
            {
                case 0u:
                {
                    highp float _1428 = _1424.x;
                    if (_1428 == 0.0)
                    {
                        _1438 = vec3(0.0);
                        break;
                    }
                    _1438 = (((_1425 * _1424.w) / vec3(_1428)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1440 = 1.0 + (_1309 * _1309);
            _1450 = vec3(_1345.x ? vec3(0.0).x : _1344.x, _1345.y ? vec3(0.0).y : _1344.y, _1345.z ? vec3(0.0).z : _1344.z);
            _1451 = (_1425 * (0.0596831031143665313720703125 * _1440)) + (_1438 * ((0.01627720706164836883544921875 * _1440) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1309), 1.5)));
            break;
        }
    }
    highp vec3 _1459;
    if (dot(_128, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1459 = _1451 + (_1450 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1459 = _1451;
    }
    highp vec3 _1477 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1459, _1278, vec3(float(_195))), _800, vec3(float(_211) * min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _141))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1479 = vec4(_1477.x, _1477.y, _1477.z, _107.w);
    _1479.w = 1.0;
    out_var_SV_Target0 = _1479;
    out_var_SV_Target1 = _136;
    gl_FragDepth = _141;
}

