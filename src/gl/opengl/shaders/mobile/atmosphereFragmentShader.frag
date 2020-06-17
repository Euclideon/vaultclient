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
    highp vec4 u_earthNorth;
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

vec4 _110;

void main()
{
    highp float _118 = u_fragParams.u_earthCenter.w + 60000.0;
    highp vec3 _131 = normalize(varying_TEXCOORD1);
    highp vec4 _135 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0);
    highp vec4 _139 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, varying_TEXCOORD0);
    highp vec4 _143 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0);
    highp float _144 = _143.x;
    highp float _149 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp vec2 _167 = _139.xy;
    highp vec3 _175 = pow(abs(_135.xyz), vec3(2.2000000476837158203125));
    highp float _181 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _149) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _144 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _149))) * u_cameraPlaneParams.s_CameraFarPlane;
    highp vec3 _183 = u_fragParams.u_camera.xyz + (_131 * _181);
    highp vec3 _184 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    highp float _185 = dot(_184, _131);
    highp float _187 = _185 * _185;
    highp float _189 = -_185;
    highp float _190 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _193 = _189 - sqrt(_190 - (dot(_184, _184) - _187));
    bool _194 = _193 > 0.0;
    highp vec3 _199;
    if (_194)
    {
        _199 = u_fragParams.u_camera.xyz + (_131 * _193);
    }
    else
    {
        _199 = _183;
    }
    highp vec3 _203 = _199 - u_fragParams.u_earthCenter.xyz;
    highp vec3 _204 = normalize(_203);
    highp vec3 _212 = vec3(_139.xy, float(int(sign(_139.w))) * sqrt(1.0 - dot(_167, _167)));
    _212.y = _139.y * (-1.0);
    highp vec3 _213 = mat3(normalize(cross(u_fragParams.u_earthNorth.xyz, _204)), u_fragParams.u_earthNorth.xyz, _204) * _212;
    bool _214 = _144 < 0.75;
    highp vec3 _802;
    if (_214)
    {
        highp vec3 _217 = _183 - u_fragParams.u_earthCenter.xyz;
        highp float _220 = length(_217);
        highp float _222 = dot(_217, u_fragParams.u_sunDirection.xyz) / _220;
        highp float _224 = _118 - u_fragParams.u_earthCenter.w;
        highp vec4 _235 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_222 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_220 - u_fragParams.u_earthCenter.w) / _224) * 0.9375)));
        highp float _242 = u_fragParams.u_earthCenter.w / _220;
        highp float _248 = _118 * _118;
        highp float _250 = sqrt(_248 - _190);
        highp float _251 = _220 * _220;
        highp float _254 = sqrt(max(_251 - _190, 0.0));
        highp float _265 = _118 - _220;
        highp vec4 _278 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_220) * _222) + sqrt(max((_251 * ((_222 * _222) - 1.0)) + _248, 0.0)), 0.0) - _265) / ((_254 + _250) - _265)) * 0.99609375), 0.0078125 + ((_254 / _250) * 0.984375)));
        highp float _295 = max(0.0, min(0.0, _181));
        highp vec3 _298 = normalize(_217 - _184);
        highp float _299 = length(_184);
        highp float _300 = dot(_184, _298);
        highp float _307 = (-_300) - sqrt(((_300 * _300) - (_299 * _299)) + _248);
        bool _308 = _307 > 0.0;
        highp vec3 _314;
        highp float _315;
        if (_308)
        {
            _314 = _184 + (_298 * _307);
            _315 = _300 + _307;
        }
        else
        {
            _314 = _184;
            _315 = _300;
        }
        highp float _334;
        highp float _316 = _308 ? _118 : _299;
        highp float _317 = _315 / _316;
        highp float _318 = dot(_314, u_fragParams.u_sunDirection.xyz);
        highp float _319 = _318 / _316;
        highp float _320 = dot(_298, u_fragParams.u_sunDirection.xyz);
        highp float _322 = length(_217 - _314);
        highp float _324 = _316 * _316;
        highp float _327 = _324 * ((_317 * _317) - 1.0);
        bool _330 = (_317 < 0.0) && ((_327 + _190) >= 0.0);
        highp vec3 _459;
        switch (0u)
        {
            case 0u:
            {
                _334 = (2.0 * _316) * _317;
                highp float _339 = clamp(sqrt((_322 * (_322 + _334)) + _324), u_fragParams.u_earthCenter.w, _118);
                highp float _342 = clamp((_315 + _322) / _339, -1.0, 1.0);
                if (_330)
                {
                    highp float _400 = -_342;
                    highp float _401 = _339 * _339;
                    highp float _404 = sqrt(max(_401 - _190, 0.0));
                    highp float _415 = _118 - _339;
                    highp float _429 = -_317;
                    highp float _432 = sqrt(max(_324 - _190, 0.0));
                    highp float _443 = _118 - _316;
                    _459 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_339) * _400) + sqrt(max((_401 * ((_400 * _400) - 1.0)) + _248, 0.0)), 0.0) - _415) / ((_404 + _250) - _415)) * 0.99609375), 0.0078125 + ((_404 / _250) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_316) * _429) + sqrt(max((_324 * ((_429 * _429) - 1.0)) + _248, 0.0)), 0.0) - _443) / ((_432 + _250) - _443)) * 0.99609375), 0.0078125 + ((_432 / _250) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _348 = sqrt(max(_324 - _190, 0.0));
                    highp float _356 = _118 - _316;
                    highp float _370 = _339 * _339;
                    highp float _373 = sqrt(max(_370 - _190, 0.0));
                    highp float _384 = _118 - _339;
                    _459 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_316) * _317) + sqrt(max(_327 + _248, 0.0)), 0.0) - _356) / ((_348 + _250) - _356)) * 0.99609375), 0.0078125 + ((_348 / _250) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_339) * _342) + sqrt(max((_370 * ((_342 * _342) - 1.0)) + _248, 0.0)), 0.0) - _384) / ((_373 + _250) - _384)) * 0.99609375), 0.0078125 + ((_373 / _250) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _462 = sqrt(max(_324 - _190, 0.0));
        highp float _463 = _462 / _250;
        highp float _465 = 0.015625 + (_463 * 0.96875);
        highp float _468 = ((_315 * _315) - _324) + _190;
        highp float _501;
        if (_330)
        {
            highp float _491 = _316 - u_fragParams.u_earthCenter.w;
            _501 = 0.5 - (0.5 * (0.0078125 + (((_462 == _491) ? 0.0 : ((((-_315) - sqrt(max(_468, 0.0))) - _491) / (_462 - _491))) * 0.984375)));
        }
        else
        {
            highp float _478 = _118 - _316;
            _501 = 0.5 + (0.5 * (0.0078125 + (((((-_315) + sqrt(max(_468 + (_250 * _250), 0.0))) - _478) / ((_462 + _250) - _478)) * 0.984375)));
        }
        highp float _506 = -u_fragParams.u_earthCenter.w;
        highp float _513 = _250 - _224;
        highp float _514 = (max((_506 * _319) + sqrt(max((_190 * ((_319 * _319) - 1.0)) + _248, 0.0)), 0.0) - _224) / _513;
        highp float _516 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _513;
        highp float _523 = 0.015625 + ((max(1.0 - (_514 / _516), 0.0) / (1.0 + _514)) * 0.96875);
        highp float _525 = (_320 + 1.0) * 3.5;
        highp float _526 = floor(_525);
        highp float _527 = _525 - _526;
        highp float _531 = _526 + 1.0;
        highp vec4 _537 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_526 + _523) * 0.125, _501, _465));
        highp float _538 = 1.0 - _527;
        highp vec4 _541 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_531 + _523) * 0.125, _501, _465));
        highp vec4 _543 = (_537 * _538) + (_541 * _527);
        highp vec3 _544 = _543.xyz;
        highp vec3 _557;
        switch (0u)
        {
            case 0u:
            {
                highp float _547 = _543.x;
                if (_547 == 0.0)
                {
                    _557 = vec3(0.0);
                    break;
                }
                _557 = (((_544 * _543.w) / vec3(_547)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _559 = max(_322 - _295, 0.0);
        highp float _564 = clamp(sqrt((_559 * (_559 + _334)) + _324), u_fragParams.u_earthCenter.w, _118);
        highp float _565 = _315 + _559;
        highp float _568 = (_318 + (_559 * _320)) / _564;
        highp float _569 = _564 * _564;
        highp float _572 = sqrt(max(_569 - _190, 0.0));
        highp float _573 = _572 / _250;
        highp float _575 = 0.015625 + (_573 * 0.96875);
        highp float _578 = ((_565 * _565) - _569) + _190;
        highp float _611;
        if (_330)
        {
            highp float _601 = _564 - u_fragParams.u_earthCenter.w;
            _611 = 0.5 - (0.5 * (0.0078125 + (((_572 == _601) ? 0.0 : ((((-_565) - sqrt(max(_578, 0.0))) - _601) / (_572 - _601))) * 0.984375)));
        }
        else
        {
            highp float _588 = _118 - _564;
            _611 = 0.5 + (0.5 * (0.0078125 + (((((-_565) + sqrt(max(_578 + (_250 * _250), 0.0))) - _588) / ((_572 + _250) - _588)) * 0.984375)));
        }
        highp float _622 = (max((_506 * _568) + sqrt(max((_190 * ((_568 * _568) - 1.0)) + _248, 0.0)), 0.0) - _224) / _513;
        highp float _629 = 0.015625 + ((max(1.0 - (_622 / _516), 0.0) / (1.0 + _622)) * 0.96875);
        highp vec4 _637 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_526 + _629) * 0.125, _611, _575));
        highp vec4 _640 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_531 + _629) * 0.125, _611, _575));
        highp vec4 _642 = (_637 * _538) + (_640 * _527);
        highp vec3 _643 = _642.xyz;
        highp vec3 _656;
        switch (0u)
        {
            case 0u:
            {
                highp float _646 = _642.x;
                if (_646 == 0.0)
                {
                    _656 = vec3(0.0);
                    break;
                }
                _656 = (((_643 * _642.w) / vec3(_646)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _763;
        if (_295 > 0.0)
        {
            highp vec3 _762;
            switch (0u)
            {
                case 0u:
                {
                    highp float _663 = clamp(_565 / _564, -1.0, 1.0);
                    if (_330)
                    {
                        highp float _712 = -_663;
                        highp float _723 = _118 - _564;
                        highp float _736 = -_317;
                        highp float _747 = _118 - _316;
                        _762 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_564) * _712) + sqrt(max((_569 * ((_712 * _712) - 1.0)) + _248, 0.0)), 0.0) - _723) / ((_572 + _250) - _723)) * 0.99609375), 0.0078125 + (_573 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_316) * _736) + sqrt(max((_324 * ((_736 * _736) - 1.0)) + _248, 0.0)), 0.0) - _747) / ((_462 + _250) - _747)) * 0.99609375), 0.0078125 + (_463 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _674 = _118 - _316;
                        highp float _697 = _118 - _564;
                        _762 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_316) * _317) + sqrt(max(_327 + _248, 0.0)), 0.0) - _674) / ((_462 + _250) - _674)) * 0.99609375), 0.0078125 + (_463 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_564) * _663) + sqrt(max((_569 * ((_663 * _663) - 1.0)) + _248, 0.0)), 0.0) - _697) / ((_572 + _250) - _697)) * 0.99609375), 0.0078125 + (_573 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _763 = _762;
        }
        else
        {
            _763 = _459;
        }
        highp vec3 _765 = _544 - (_763 * _643);
        highp vec3 _767 = _557 - (_763 * _656);
        highp float _768 = _767.x;
        highp float _769 = _765.x;
        highp vec3 _784;
        switch (0u)
        {
            case 0u:
            {
                if (_769 == 0.0)
                {
                    _784 = vec3(0.0);
                    break;
                }
                _784 = (((vec4(_769, _765.yz, _768).xyz * _768) / vec3(_769)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _788 = 1.0 + (_320 * _320);
        _802 = (((_175.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_278.xyz * smoothstep(_242 * (-0.004674999974668025970458984375), _242 * 0.004674999974668025970458984375, _222 - (-sqrt(max(1.0 - (_242 * _242), 0.0))))) * max(dot(_213, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_235.xyz * (1.0 + (dot(_213, _217) / _220))) * 0.5))) * _459) + (((_765 * (0.0596831031143665313720703125 * _788)) + ((_784 * smoothstep(0.0, 0.00999999977648258209228515625, _319)) * ((0.01627720706164836883544921875 * _788) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _320), 1.5)))) * mix(0.5, 1.0, min(1.0, pow(_144, 6.0) * 6.0)));
    }
    else
    {
        _802 = vec3(0.0);
    }
    highp vec3 _1278;
    if (_194)
    {
        highp float _808 = length(_203);
        highp float _810 = dot(_203, u_fragParams.u_sunDirection.xyz) / _808;
        highp float _812 = _118 - u_fragParams.u_earthCenter.w;
        highp vec4 _823 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_810 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_808 - u_fragParams.u_earthCenter.w) / _812) * 0.9375)));
        highp float _830 = u_fragParams.u_earthCenter.w / _808;
        highp float _836 = _118 * _118;
        highp float _838 = sqrt(_836 - _190);
        highp float _839 = _808 * _808;
        highp float _842 = sqrt(max(_839 - _190, 0.0));
        highp float _853 = _118 - _808;
        highp vec4 _866 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_808) * _810) + sqrt(max((_839 * ((_810 * _810) - 1.0)) + _836, 0.0)), 0.0) - _853) / ((_842 + _838) - _853)) * 0.99609375), 0.0078125 + ((_842 / _838) * 0.984375)));
        highp vec3 _884 = normalize(_203 - _184);
        highp float _885 = length(_184);
        highp float _886 = dot(_184, _884);
        highp float _893 = (-_886) - sqrt(((_886 * _886) - (_885 * _885)) + _836);
        bool _894 = _893 > 0.0;
        highp vec3 _900;
        highp float _901;
        if (_894)
        {
            _900 = _184 + (_884 * _893);
            _901 = _886 + _893;
        }
        else
        {
            _900 = _184;
            _901 = _886;
        }
        highp float _920;
        highp float _902 = _894 ? _118 : _885;
        highp float _903 = _901 / _902;
        highp float _904 = dot(_900, u_fragParams.u_sunDirection.xyz);
        highp float _905 = _904 / _902;
        highp float _906 = dot(_884, u_fragParams.u_sunDirection.xyz);
        highp float _908 = length(_203 - _900);
        highp float _910 = _902 * _902;
        highp float _913 = _910 * ((_903 * _903) - 1.0);
        bool _916 = (_903 < 0.0) && ((_913 + _190) >= 0.0);
        highp vec3 _1045;
        switch (0u)
        {
            case 0u:
            {
                _920 = (2.0 * _902) * _903;
                highp float _925 = clamp(sqrt((_908 * (_908 + _920)) + _910), u_fragParams.u_earthCenter.w, _118);
                highp float _928 = clamp((_901 + _908) / _925, -1.0, 1.0);
                if (_916)
                {
                    highp float _986 = -_928;
                    highp float _987 = _925 * _925;
                    highp float _990 = sqrt(max(_987 - _190, 0.0));
                    highp float _1001 = _118 - _925;
                    highp float _1015 = -_903;
                    highp float _1018 = sqrt(max(_910 - _190, 0.0));
                    highp float _1029 = _118 - _902;
                    _1045 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_925) * _986) + sqrt(max((_987 * ((_986 * _986) - 1.0)) + _836, 0.0)), 0.0) - _1001) / ((_990 + _838) - _1001)) * 0.99609375), 0.0078125 + ((_990 / _838) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_902) * _1015) + sqrt(max((_910 * ((_1015 * _1015) - 1.0)) + _836, 0.0)), 0.0) - _1029) / ((_1018 + _838) - _1029)) * 0.99609375), 0.0078125 + ((_1018 / _838) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _934 = sqrt(max(_910 - _190, 0.0));
                    highp float _942 = _118 - _902;
                    highp float _956 = _925 * _925;
                    highp float _959 = sqrt(max(_956 - _190, 0.0));
                    highp float _970 = _118 - _925;
                    _1045 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_902) * _903) + sqrt(max(_913 + _836, 0.0)), 0.0) - _942) / ((_934 + _838) - _942)) * 0.99609375), 0.0078125 + ((_934 / _838) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_925) * _928) + sqrt(max((_956 * ((_928 * _928) - 1.0)) + _836, 0.0)), 0.0) - _970) / ((_959 + _838) - _970)) * 0.99609375), 0.0078125 + ((_959 / _838) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _1048 = sqrt(max(_910 - _190, 0.0));
        highp float _1051 = 0.015625 + ((_1048 / _838) * 0.96875);
        highp float _1054 = ((_901 * _901) - _910) + _190;
        highp float _1087;
        if (_916)
        {
            highp float _1077 = _902 - u_fragParams.u_earthCenter.w;
            _1087 = 0.5 - (0.5 * (0.0078125 + (((_1048 == _1077) ? 0.0 : ((((-_901) - sqrt(max(_1054, 0.0))) - _1077) / (_1048 - _1077))) * 0.984375)));
        }
        else
        {
            highp float _1064 = _118 - _902;
            _1087 = 0.5 + (0.5 * (0.0078125 + (((((-_901) + sqrt(max(_1054 + (_838 * _838), 0.0))) - _1064) / ((_1048 + _838) - _1064)) * 0.984375)));
        }
        highp float _1092 = -u_fragParams.u_earthCenter.w;
        highp float _1099 = _838 - _812;
        highp float _1100 = (max((_1092 * _905) + sqrt(max((_190 * ((_905 * _905) - 1.0)) + _836, 0.0)), 0.0) - _812) / _1099;
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
        highp float _1149 = clamp(sqrt((_1144 * (_1144 + _920)) + _910), u_fragParams.u_earthCenter.w, _118);
        highp float _1150 = _901 + _1144;
        highp float _1153 = (_904 + (_1144 * _906)) / _1149;
        highp float _1154 = _1149 * _1149;
        highp float _1157 = sqrt(max(_1154 - _190, 0.0));
        highp float _1160 = 0.015625 + ((_1157 / _838) * 0.96875);
        highp float _1163 = ((_1150 * _1150) - _1154) + _190;
        highp float _1196;
        if (_916)
        {
            highp float _1186 = _1149 - u_fragParams.u_earthCenter.w;
            _1196 = 0.5 - (0.5 * (0.0078125 + (((_1157 == _1186) ? 0.0 : ((((-_1150) - sqrt(max(_1163, 0.0))) - _1186) / (_1157 - _1186))) * 0.984375)));
        }
        else
        {
            highp float _1173 = _118 - _1149;
            _1196 = 0.5 + (0.5 * (0.0078125 + (((((-_1150) + sqrt(max(_1163 + (_838 * _838), 0.0))) - _1173) / ((_1157 + _838) - _1173)) * 0.984375)));
        }
        highp float _1207 = (max((_1092 * _1153) + sqrt(max((_190 * ((_1153 * _1153) - 1.0)) + _836, 0.0)), 0.0) - _812) / _1099;
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
        _1278 = (((_175.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_866.xyz * smoothstep(_830 * (-0.004674999974668025970458984375), _830 * 0.004674999974668025970458984375, _810 - (-sqrt(max(1.0 - (_830 * _830), 0.0))))) * max(dot(_213, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_823.xyz * (1.0 + (dot(_213, _203) / _808))) * 0.5))) * _1045) + ((_1243 * (0.0596831031143665313720703125 * _1266)) + ((_1262 * smoothstep(0.0, 0.00999999977648258209228515625, _905)) * ((0.01627720706164836883544921875 * _1266) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _906), 1.5))));
    }
    else
    {
        _1278 = vec3(0.0);
    }
    highp float _1284;
    highp vec3 _1448;
    highp vec3 _1449;
    switch (0u)
    {
        case 0u:
        {
            _1284 = length(_184);
            highp float _1287 = _118 * _118;
            highp float _1290 = _189 - sqrt((_187 - (_1284 * _1284)) + _1287);
            bool _1291 = _1290 > 0.0;
            highp vec3 _1301;
            highp float _1302;
            if (_1291)
            {
                _1301 = _184 + (_131 * _1290);
                _1302 = _185 + _1290;
            }
            else
            {
                if (_1284 > _118)
                {
                    _1448 = vec3(1.0);
                    _1449 = vec3(0.0);
                    break;
                }
                _1301 = _184;
                _1302 = _185;
            }
            highp float _1303 = _1291 ? _118 : _1284;
            highp float _1304 = _1302 / _1303;
            highp float _1306 = dot(_1301, u_fragParams.u_sunDirection.xyz) / _1303;
            highp float _1307 = dot(_131, u_fragParams.u_sunDirection.xyz);
            highp float _1309 = _1303 * _1303;
            highp float _1312 = _1309 * ((_1304 * _1304) - 1.0);
            bool _1315 = (_1304 < 0.0) && ((_1312 + _190) >= 0.0);
            highp float _1317 = sqrt(_1287 - _190);
            highp float _1320 = sqrt(max(_1309 - _190, 0.0));
            highp float _1328 = _118 - _1303;
            highp float _1331 = (_1320 + _1317) - _1328;
            highp float _1333 = _1320 / _1317;
            highp vec4 _1341 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1303) * _1304) + sqrt(max(_1312 + _1287, 0.0)), 0.0) - _1328) / _1331) * 0.99609375), 0.0078125 + (_1333 * 0.984375)));
            highp vec3 _1342 = _1341.xyz;
            bvec3 _1343 = bvec3(_1315);
            highp float _1346 = 0.015625 + (_1333 * 0.96875);
            highp float _1349 = ((_1302 * _1302) - _1309) + _190;
            highp float _1379;
            if (_1315)
            {
                highp float _1369 = _1303 - u_fragParams.u_earthCenter.w;
                _1379 = 0.5 - (0.5 * (0.0078125 + (((_1320 == _1369) ? 0.0 : ((((-_1302) - sqrt(max(_1349, 0.0))) - _1369) / (_1320 - _1369))) * 0.984375)));
            }
            else
            {
                _1379 = 0.5 + (0.5 * (0.0078125 + (((((-_1302) + sqrt(max(_1349 + (_1317 * _1317), 0.0))) - _1328) / _1331) * 0.984375)));
            }
            highp float _1390 = _118 - u_fragParams.u_earthCenter.w;
            highp float _1392 = _1317 - _1390;
            highp float _1393 = (max(((-u_fragParams.u_earthCenter.w) * _1306) + sqrt(max((_190 * ((_1306 * _1306) - 1.0)) + _1287, 0.0)), 0.0) - _1390) / _1392;
            highp float _1402 = 0.015625 + ((max(1.0 - (_1393 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1392)), 0.0) / (1.0 + _1393)) * 0.96875);
            highp float _1404 = (_1307 + 1.0) * 3.5;
            highp float _1405 = floor(_1404);
            highp float _1406 = _1404 - _1405;
            highp vec4 _1416 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1405 + _1402) * 0.125, _1379, _1346));
            highp vec4 _1420 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1405 + 1.0) + _1402) * 0.125, _1379, _1346));
            highp vec4 _1422 = (_1416 * (1.0 - _1406)) + (_1420 * _1406);
            highp vec3 _1423 = _1422.xyz;
            highp vec3 _1436;
            switch (0u)
            {
                case 0u:
                {
                    highp float _1426 = _1422.x;
                    if (_1426 == 0.0)
                    {
                        _1436 = vec3(0.0);
                        break;
                    }
                    _1436 = (((_1423 * _1422.w) / vec3(_1426)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1438 = 1.0 + (_1307 * _1307);
            _1448 = vec3(_1343.x ? vec3(0.0).x : _1342.x, _1343.y ? vec3(0.0).y : _1342.y, _1343.z ? vec3(0.0).z : _1342.z);
            _1449 = (_1423 * (0.0596831031143665313720703125 * _1438)) + (_1436 * ((0.01627720706164836883544921875 * _1438) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1307), 1.5)));
            break;
        }
    }
    highp vec3 _1457;
    if (dot(_131, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1457 = _1449 + (_1448 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1457 = _1449;
    }
    highp vec3 _1475 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1457, _1278, vec3(float(_194))), _802, vec3(float(_214) * min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _144))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1477 = vec4(_1475.x, _1475.y, _1475.z, _110.w);
    _1477.w = 1.0;
    highp vec4 _1486;
    if ((_1284 < u_fragParams.u_earthCenter.w) && (_185 <= 0.0))
    {
        highp vec3 _1484 = pow(_175.xyz, vec3(0.5));
        _1486 = vec4(_1484.x, _1484.y, _1484.z, _1477.w);
    }
    else
    {
        _1486 = _1477;
    }
    out_var_SV_Target0 = _1486;
    out_var_SV_Target1 = _139;
    gl_FragDepth = _144;
}

