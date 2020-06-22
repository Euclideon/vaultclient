#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_fragParams
{
    vec4 u_camera;
    vec4 u_whitePoint;
    vec4 u_earthCenter;
    vec4 u_sunDirection;
    vec4 u_sunSize;
    vec4 u_earthNorth;
} u_fragParams;

uniform sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform sampler2D SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler;
uniform sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform sampler2D SPIRV_Cross_CombinedirradianceTextureirradianceSampler;
uniform sampler2D SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler;
uniform sampler3D SPIRV_Cross_CombinedscatteringTexturescatteringSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec3 in_var_TEXCOORD1;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

vec4 _109;

void main()
{
    float _117 = u_fragParams.u_earthCenter.w + 60000.0;
    vec3 _130 = normalize(in_var_TEXCOORD1);
    vec4 _134 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD0);
    vec4 _138 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, in_var_TEXCOORD0);
    vec4 _142 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD0);
    float _143 = _142.x;
    float _148 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    vec2 _166 = _138.xy;
    vec3 _174 = pow(abs(_134.xyz), vec3(2.2000000476837158203125));
    float _180 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _148) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _143 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _148))) * u_cameraPlaneParams.s_CameraFarPlane;
    vec3 _182 = u_fragParams.u_camera.xyz + (_130 * _180);
    vec3 _183 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    float _184 = dot(_183, _130);
    float _186 = _184 * _184;
    float _188 = -_184;
    float _189 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    float _192 = _188 - sqrt(_189 - (dot(_183, _183) - _186));
    bool _193 = _192 > 0.0;
    vec3 _198;
    if (_193)
    {
        _198 = u_fragParams.u_camera.xyz + (_130 * _192);
    }
    else
    {
        _198 = _182;
    }
    vec3 _202 = _198 - u_fragParams.u_earthCenter.xyz;
    vec3 _203 = normalize(_202);
    vec3 _211 = vec3(_138.xy, float(int(sign(_138.w))) * sqrt(1.0 - dot(_166, _166)));
    _211.y = _138.y * (-1.0);
    vec3 _212 = mat3(normalize(cross(u_fragParams.u_earthNorth.xyz, _203)), u_fragParams.u_earthNorth.xyz, _203) * _211;
    bool _213 = _143 < 0.75;
    vec3 _801;
    if (_213)
    {
        vec3 _216 = _182 - u_fragParams.u_earthCenter.xyz;
        float _219 = length(_216);
        float _221 = dot(_216, u_fragParams.u_sunDirection.xyz) / _219;
        float _223 = _117 - u_fragParams.u_earthCenter.w;
        vec4 _234 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_221 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_219 - u_fragParams.u_earthCenter.w) / _223) * 0.9375)));
        float _241 = u_fragParams.u_earthCenter.w / _219;
        float _247 = _117 * _117;
        float _249 = sqrt(_247 - _189);
        float _250 = _219 * _219;
        float _253 = sqrt(max(_250 - _189, 0.0));
        float _264 = _117 - _219;
        vec4 _277 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_219) * _221) + sqrt(max((_250 * ((_221 * _221) - 1.0)) + _247, 0.0)), 0.0) - _264) / ((_253 + _249) - _264)) * 0.99609375), 0.0078125 + ((_253 / _249) * 0.984375)));
        float _294 = max(0.0, min(0.0, _180));
        vec3 _297 = normalize(_216 - _183);
        float _298 = length(_183);
        float _299 = dot(_183, _297);
        float _306 = (-_299) - sqrt(((_299 * _299) - (_298 * _298)) + _247);
        bool _307 = _306 > 0.0;
        vec3 _313;
        float _314;
        if (_307)
        {
            _313 = _183 + (_297 * _306);
            _314 = _299 + _306;
        }
        else
        {
            _313 = _183;
            _314 = _299;
        }
        float _333;
        float _315 = _307 ? _117 : _298;
        float _316 = _314 / _315;
        float _317 = dot(_313, u_fragParams.u_sunDirection.xyz);
        float _318 = _317 / _315;
        float _319 = dot(_297, u_fragParams.u_sunDirection.xyz);
        float _321 = length(_216 - _313);
        float _323 = _315 * _315;
        float _326 = _323 * ((_316 * _316) - 1.0);
        bool _329 = (_316 < 0.0) && ((_326 + _189) >= 0.0);
        vec3 _458;
        switch (0u)
        {
            default:
            {
                _333 = (2.0 * _315) * _316;
                float _338 = clamp(sqrt((_321 * (_321 + _333)) + _323), u_fragParams.u_earthCenter.w, _117);
                float _341 = clamp((_314 + _321) / _338, -1.0, 1.0);
                if (_329)
                {
                    float _399 = -_341;
                    float _400 = _338 * _338;
                    float _403 = sqrt(max(_400 - _189, 0.0));
                    float _414 = _117 - _338;
                    float _428 = -_316;
                    float _431 = sqrt(max(_323 - _189, 0.0));
                    float _442 = _117 - _315;
                    _458 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_338) * _399) + sqrt(max((_400 * ((_399 * _399) - 1.0)) + _247, 0.0)), 0.0) - _414) / ((_403 + _249) - _414)) * 0.99609375), 0.0078125 + ((_403 / _249) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_315) * _428) + sqrt(max((_323 * ((_428 * _428) - 1.0)) + _247, 0.0)), 0.0) - _442) / ((_431 + _249) - _442)) * 0.99609375), 0.0078125 + ((_431 / _249) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    float _347 = sqrt(max(_323 - _189, 0.0));
                    float _355 = _117 - _315;
                    float _369 = _338 * _338;
                    float _372 = sqrt(max(_369 - _189, 0.0));
                    float _383 = _117 - _338;
                    _458 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_315) * _316) + sqrt(max(_326 + _247, 0.0)), 0.0) - _355) / ((_347 + _249) - _355)) * 0.99609375), 0.0078125 + ((_347 / _249) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_338) * _341) + sqrt(max((_369 * ((_341 * _341) - 1.0)) + _247, 0.0)), 0.0) - _383) / ((_372 + _249) - _383)) * 0.99609375), 0.0078125 + ((_372 / _249) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        float _461 = sqrt(max(_323 - _189, 0.0));
        float _462 = _461 / _249;
        float _464 = 0.015625 + (_462 * 0.96875);
        float _467 = ((_314 * _314) - _323) + _189;
        float _500;
        if (_329)
        {
            float _490 = _315 - u_fragParams.u_earthCenter.w;
            _500 = 0.5 - (0.5 * (0.0078125 + (((_461 == _490) ? 0.0 : ((((-_314) - sqrt(max(_467, 0.0))) - _490) / (_461 - _490))) * 0.984375)));
        }
        else
        {
            float _477 = _117 - _315;
            _500 = 0.5 + (0.5 * (0.0078125 + (((((-_314) + sqrt(max(_467 + (_249 * _249), 0.0))) - _477) / ((_461 + _249) - _477)) * 0.984375)));
        }
        float _505 = -u_fragParams.u_earthCenter.w;
        float _512 = _249 - _223;
        float _513 = (max((_505 * _318) + sqrt(max((_189 * ((_318 * _318) - 1.0)) + _247, 0.0)), 0.0) - _223) / _512;
        float _515 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _512;
        float _522 = 0.015625 + ((max(1.0 - (_513 / _515), 0.0) / (1.0 + _513)) * 0.96875);
        float _524 = (_319 + 1.0) * 3.5;
        float _525 = floor(_524);
        float _526 = _524 - _525;
        float _530 = _525 + 1.0;
        vec4 _536 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_525 + _522) * 0.125, _500, _464));
        float _537 = 1.0 - _526;
        vec4 _540 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_530 + _522) * 0.125, _500, _464));
        vec4 _542 = (_536 * _537) + (_540 * _526);
        vec3 _543 = _542.xyz;
        vec3 _556;
        switch (0u)
        {
            default:
            {
                float _546 = _542.x;
                if (_546 == 0.0)
                {
                    _556 = vec3(0.0);
                    break;
                }
                _556 = (((_543 * _542.w) / vec3(_546)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _558 = max(_321 - _294, 0.0);
        float _563 = clamp(sqrt((_558 * (_558 + _333)) + _323), u_fragParams.u_earthCenter.w, _117);
        float _564 = _314 + _558;
        float _567 = (_317 + (_558 * _319)) / _563;
        float _568 = _563 * _563;
        float _571 = sqrt(max(_568 - _189, 0.0));
        float _572 = _571 / _249;
        float _574 = 0.015625 + (_572 * 0.96875);
        float _577 = ((_564 * _564) - _568) + _189;
        float _610;
        if (_329)
        {
            float _600 = _563 - u_fragParams.u_earthCenter.w;
            _610 = 0.5 - (0.5 * (0.0078125 + (((_571 == _600) ? 0.0 : ((((-_564) - sqrt(max(_577, 0.0))) - _600) / (_571 - _600))) * 0.984375)));
        }
        else
        {
            float _587 = _117 - _563;
            _610 = 0.5 + (0.5 * (0.0078125 + (((((-_564) + sqrt(max(_577 + (_249 * _249), 0.0))) - _587) / ((_571 + _249) - _587)) * 0.984375)));
        }
        float _621 = (max((_505 * _567) + sqrt(max((_189 * ((_567 * _567) - 1.0)) + _247, 0.0)), 0.0) - _223) / _512;
        float _628 = 0.015625 + ((max(1.0 - (_621 / _515), 0.0) / (1.0 + _621)) * 0.96875);
        vec4 _636 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_525 + _628) * 0.125, _610, _574));
        vec4 _639 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_530 + _628) * 0.125, _610, _574));
        vec4 _641 = (_636 * _537) + (_639 * _526);
        vec3 _642 = _641.xyz;
        vec3 _655;
        switch (0u)
        {
            default:
            {
                float _645 = _641.x;
                if (_645 == 0.0)
                {
                    _655 = vec3(0.0);
                    break;
                }
                _655 = (((_642 * _641.w) / vec3(_645)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        vec3 _762;
        if (_294 > 0.0)
        {
            vec3 _761;
            switch (0u)
            {
                default:
                {
                    float _662 = clamp(_564 / _563, -1.0, 1.0);
                    if (_329)
                    {
                        float _711 = -_662;
                        float _722 = _117 - _563;
                        float _735 = -_316;
                        float _746 = _117 - _315;
                        _761 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_563) * _711) + sqrt(max((_568 * ((_711 * _711) - 1.0)) + _247, 0.0)), 0.0) - _722) / ((_571 + _249) - _722)) * 0.99609375), 0.0078125 + (_572 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_315) * _735) + sqrt(max((_323 * ((_735 * _735) - 1.0)) + _247, 0.0)), 0.0) - _746) / ((_461 + _249) - _746)) * 0.99609375), 0.0078125 + (_462 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        float _673 = _117 - _315;
                        float _696 = _117 - _563;
                        _761 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_315) * _316) + sqrt(max(_326 + _247, 0.0)), 0.0) - _673) / ((_461 + _249) - _673)) * 0.99609375), 0.0078125 + (_462 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_563) * _662) + sqrt(max((_568 * ((_662 * _662) - 1.0)) + _247, 0.0)), 0.0) - _696) / ((_571 + _249) - _696)) * 0.99609375), 0.0078125 + (_572 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _762 = _761;
        }
        else
        {
            _762 = _458;
        }
        vec3 _764 = _543 - (_762 * _642);
        vec3 _766 = _556 - (_762 * _655);
        float _767 = _766.x;
        float _768 = _764.x;
        vec3 _783;
        switch (0u)
        {
            default:
            {
                if (_768 == 0.0)
                {
                    _783 = vec3(0.0);
                    break;
                }
                _783 = (((vec4(_768, _764.yz, _767).xyz * _767) / vec3(_768)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _787 = 1.0 + (_319 * _319);
        _801 = (((_174.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_277.xyz * smoothstep(_241 * (-0.004674999974668025970458984375), _241 * 0.004674999974668025970458984375, _221 - (-sqrt(max(1.0 - (_241 * _241), 0.0))))) * max(dot(_212, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_234.xyz * (1.0 + (dot(_212, _216) / _219))) * 0.5))) * _458) + (((_764 * (0.0596831031143665313720703125 * _787)) + ((_783 * smoothstep(0.0, 0.00999999977648258209228515625, _318)) * ((0.01627720706164836883544921875 * _787) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _319), 1.5)))) * mix(0.5, 1.0, min(1.0, pow(_143, 6.0) * 6.0)));
    }
    else
    {
        _801 = vec3(0.0);
    }
    vec3 _1277;
    if (_193)
    {
        float _807 = length(_202);
        float _809 = dot(_202, u_fragParams.u_sunDirection.xyz) / _807;
        float _811 = _117 - u_fragParams.u_earthCenter.w;
        vec4 _822 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_809 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_807 - u_fragParams.u_earthCenter.w) / _811) * 0.9375)));
        float _829 = u_fragParams.u_earthCenter.w / _807;
        float _835 = _117 * _117;
        float _837 = sqrt(_835 - _189);
        float _838 = _807 * _807;
        float _841 = sqrt(max(_838 - _189, 0.0));
        float _852 = _117 - _807;
        vec4 _865 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_807) * _809) + sqrt(max((_838 * ((_809 * _809) - 1.0)) + _835, 0.0)), 0.0) - _852) / ((_841 + _837) - _852)) * 0.99609375), 0.0078125 + ((_841 / _837) * 0.984375)));
        vec3 _883 = normalize(_202 - _183);
        float _884 = length(_183);
        float _885 = dot(_183, _883);
        float _892 = (-_885) - sqrt(((_885 * _885) - (_884 * _884)) + _835);
        bool _893 = _892 > 0.0;
        vec3 _899;
        float _900;
        if (_893)
        {
            _899 = _183 + (_883 * _892);
            _900 = _885 + _892;
        }
        else
        {
            _899 = _183;
            _900 = _885;
        }
        float _919;
        float _901 = _893 ? _117 : _884;
        float _902 = _900 / _901;
        float _903 = dot(_899, u_fragParams.u_sunDirection.xyz);
        float _904 = _903 / _901;
        float _905 = dot(_883, u_fragParams.u_sunDirection.xyz);
        float _907 = length(_202 - _899);
        float _909 = _901 * _901;
        float _912 = _909 * ((_902 * _902) - 1.0);
        bool _915 = (_902 < 0.0) && ((_912 + _189) >= 0.0);
        vec3 _1044;
        switch (0u)
        {
            default:
            {
                _919 = (2.0 * _901) * _902;
                float _924 = clamp(sqrt((_907 * (_907 + _919)) + _909), u_fragParams.u_earthCenter.w, _117);
                float _927 = clamp((_900 + _907) / _924, -1.0, 1.0);
                if (_915)
                {
                    float _985 = -_927;
                    float _986 = _924 * _924;
                    float _989 = sqrt(max(_986 - _189, 0.0));
                    float _1000 = _117 - _924;
                    float _1014 = -_902;
                    float _1017 = sqrt(max(_909 - _189, 0.0));
                    float _1028 = _117 - _901;
                    _1044 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_924) * _985) + sqrt(max((_986 * ((_985 * _985) - 1.0)) + _835, 0.0)), 0.0) - _1000) / ((_989 + _837) - _1000)) * 0.99609375), 0.0078125 + ((_989 / _837) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_901) * _1014) + sqrt(max((_909 * ((_1014 * _1014) - 1.0)) + _835, 0.0)), 0.0) - _1028) / ((_1017 + _837) - _1028)) * 0.99609375), 0.0078125 + ((_1017 / _837) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    float _933 = sqrt(max(_909 - _189, 0.0));
                    float _941 = _117 - _901;
                    float _955 = _924 * _924;
                    float _958 = sqrt(max(_955 - _189, 0.0));
                    float _969 = _117 - _924;
                    _1044 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_901) * _902) + sqrt(max(_912 + _835, 0.0)), 0.0) - _941) / ((_933 + _837) - _941)) * 0.99609375), 0.0078125 + ((_933 / _837) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_924) * _927) + sqrt(max((_955 * ((_927 * _927) - 1.0)) + _835, 0.0)), 0.0) - _969) / ((_958 + _837) - _969)) * 0.99609375), 0.0078125 + ((_958 / _837) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        float _1047 = sqrt(max(_909 - _189, 0.0));
        float _1050 = 0.015625 + ((_1047 / _837) * 0.96875);
        float _1053 = ((_900 * _900) - _909) + _189;
        float _1086;
        if (_915)
        {
            float _1076 = _901 - u_fragParams.u_earthCenter.w;
            _1086 = 0.5 - (0.5 * (0.0078125 + (((_1047 == _1076) ? 0.0 : ((((-_900) - sqrt(max(_1053, 0.0))) - _1076) / (_1047 - _1076))) * 0.984375)));
        }
        else
        {
            float _1063 = _117 - _901;
            _1086 = 0.5 + (0.5 * (0.0078125 + (((((-_900) + sqrt(max(_1053 + (_837 * _837), 0.0))) - _1063) / ((_1047 + _837) - _1063)) * 0.984375)));
        }
        float _1091 = -u_fragParams.u_earthCenter.w;
        float _1098 = _837 - _811;
        float _1099 = (max((_1091 * _904) + sqrt(max((_189 * ((_904 * _904) - 1.0)) + _835, 0.0)), 0.0) - _811) / _1098;
        float _1101 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1098;
        float _1108 = 0.015625 + ((max(1.0 - (_1099 / _1101), 0.0) / (1.0 + _1099)) * 0.96875);
        float _1110 = (_905 + 1.0) * 3.5;
        float _1111 = floor(_1110);
        float _1112 = _1110 - _1111;
        float _1116 = _1111 + 1.0;
        vec4 _1122 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1111 + _1108) * 0.125, _1086, _1050));
        float _1123 = 1.0 - _1112;
        vec4 _1126 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1116 + _1108) * 0.125, _1086, _1050));
        vec4 _1128 = (_1122 * _1123) + (_1126 * _1112);
        vec3 _1129 = _1128.xyz;
        vec3 _1142;
        switch (0u)
        {
            default:
            {
                float _1132 = _1128.x;
                if (_1132 == 0.0)
                {
                    _1142 = vec3(0.0);
                    break;
                }
                _1142 = (((_1129 * _1128.w) / vec3(_1132)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1143 = max(_907, 0.0);
        float _1148 = clamp(sqrt((_1143 * (_1143 + _919)) + _909), u_fragParams.u_earthCenter.w, _117);
        float _1149 = _900 + _1143;
        float _1152 = (_903 + (_1143 * _905)) / _1148;
        float _1153 = _1148 * _1148;
        float _1156 = sqrt(max(_1153 - _189, 0.0));
        float _1159 = 0.015625 + ((_1156 / _837) * 0.96875);
        float _1162 = ((_1149 * _1149) - _1153) + _189;
        float _1195;
        if (_915)
        {
            float _1185 = _1148 - u_fragParams.u_earthCenter.w;
            _1195 = 0.5 - (0.5 * (0.0078125 + (((_1156 == _1185) ? 0.0 : ((((-_1149) - sqrt(max(_1162, 0.0))) - _1185) / (_1156 - _1185))) * 0.984375)));
        }
        else
        {
            float _1172 = _117 - _1148;
            _1195 = 0.5 + (0.5 * (0.0078125 + (((((-_1149) + sqrt(max(_1162 + (_837 * _837), 0.0))) - _1172) / ((_1156 + _837) - _1172)) * 0.984375)));
        }
        float _1206 = (max((_1091 * _1152) + sqrt(max((_189 * ((_1152 * _1152) - 1.0)) + _835, 0.0)), 0.0) - _811) / _1098;
        float _1213 = 0.015625 + ((max(1.0 - (_1206 / _1101), 0.0) / (1.0 + _1206)) * 0.96875);
        vec4 _1221 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1111 + _1213) * 0.125, _1195, _1159));
        vec4 _1224 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1116 + _1213) * 0.125, _1195, _1159));
        vec4 _1226 = (_1221 * _1123) + (_1224 * _1112);
        vec3 _1227 = _1226.xyz;
        vec3 _1240;
        switch (0u)
        {
            default:
            {
                float _1230 = _1226.x;
                if (_1230 == 0.0)
                {
                    _1240 = vec3(0.0);
                    break;
                }
                _1240 = (((_1227 * _1226.w) / vec3(_1230)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        vec3 _1242 = _1129 - (_1044 * _1227);
        vec3 _1244 = _1142 - (_1044 * _1240);
        float _1245 = _1244.x;
        float _1246 = _1242.x;
        vec3 _1261;
        switch (0u)
        {
            default:
            {
                if (_1246 == 0.0)
                {
                    _1261 = vec3(0.0);
                    break;
                }
                _1261 = (((vec4(_1246, _1242.yz, _1245).xyz * _1245) / vec3(_1246)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        float _1265 = 1.0 + (_905 * _905);
        _1277 = (((_174.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_865.xyz * smoothstep(_829 * (-0.004674999974668025970458984375), _829 * 0.004674999974668025970458984375, _809 - (-sqrt(max(1.0 - (_829 * _829), 0.0))))) * max(dot(_212, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_822.xyz * (1.0 + (dot(_212, _202) / _807))) * 0.5))) * _1044) + ((_1242 * (0.0596831031143665313720703125 * _1265)) + ((_1261 * smoothstep(0.0, 0.00999999977648258209228515625, _904)) * ((0.01627720706164836883544921875 * _1265) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _905), 1.5))));
    }
    else
    {
        _1277 = vec3(0.0);
    }
    vec3 _1447;
    vec3 _1448;
    switch (0u)
    {
        default:
        {
            float _1283 = length(_183);
            float _1286 = _117 * _117;
            float _1289 = _188 - sqrt((_186 - (_1283 * _1283)) + _1286);
            bool _1290 = _1289 > 0.0;
            vec3 _1300;
            float _1301;
            if (_1290)
            {
                _1300 = _183 + (_130 * _1289);
                _1301 = _184 + _1289;
            }
            else
            {
                if (_1283 > _117)
                {
                    _1447 = vec3(1.0);
                    _1448 = vec3(0.0);
                    break;
                }
                _1300 = _183;
                _1301 = _184;
            }
            float _1302 = _1290 ? _117 : _1283;
            float _1303 = _1301 / _1302;
            float _1305 = dot(_1300, u_fragParams.u_sunDirection.xyz) / _1302;
            float _1306 = dot(_130, u_fragParams.u_sunDirection.xyz);
            float _1308 = _1302 * _1302;
            float _1311 = _1308 * ((_1303 * _1303) - 1.0);
            bool _1314 = (_1303 < 0.0) && ((_1311 + _189) >= 0.0);
            float _1316 = sqrt(_1286 - _189);
            float _1319 = sqrt(max(_1308 - _189, 0.0));
            float _1327 = _117 - _1302;
            float _1330 = (_1319 + _1316) - _1327;
            float _1332 = _1319 / _1316;
            vec4 _1340 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1302) * _1303) + sqrt(max(_1311 + _1286, 0.0)), 0.0) - _1327) / _1330) * 0.99609375), 0.0078125 + (_1332 * 0.984375)));
            vec3 _1341 = _1340.xyz;
            bvec3 _1342 = bvec3(_1314);
            float _1345 = 0.015625 + (_1332 * 0.96875);
            float _1348 = ((_1301 * _1301) - _1308) + _189;
            float _1378;
            if (_1314)
            {
                float _1368 = _1302 - u_fragParams.u_earthCenter.w;
                _1378 = 0.5 - (0.5 * (0.0078125 + (((_1319 == _1368) ? 0.0 : ((((-_1301) - sqrt(max(_1348, 0.0))) - _1368) / (_1319 - _1368))) * 0.984375)));
            }
            else
            {
                _1378 = 0.5 + (0.5 * (0.0078125 + (((((-_1301) + sqrt(max(_1348 + (_1316 * _1316), 0.0))) - _1327) / _1330) * 0.984375)));
            }
            float _1389 = _117 - u_fragParams.u_earthCenter.w;
            float _1391 = _1316 - _1389;
            float _1392 = (max(((-u_fragParams.u_earthCenter.w) * _1305) + sqrt(max((_189 * ((_1305 * _1305) - 1.0)) + _1286, 0.0)), 0.0) - _1389) / _1391;
            float _1401 = 0.015625 + ((max(1.0 - (_1392 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1391)), 0.0) / (1.0 + _1392)) * 0.96875);
            float _1403 = (_1306 + 1.0) * 3.5;
            float _1404 = floor(_1403);
            float _1405 = _1403 - _1404;
            vec4 _1415 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1404 + _1401) * 0.125, _1378, _1345));
            vec4 _1419 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1404 + 1.0) + _1401) * 0.125, _1378, _1345));
            vec4 _1421 = (_1415 * (1.0 - _1405)) + (_1419 * _1405);
            vec3 _1422 = _1421.xyz;
            vec3 _1435;
            switch (0u)
            {
                default:
                {
                    float _1425 = _1421.x;
                    if (_1425 == 0.0)
                    {
                        _1435 = vec3(0.0);
                        break;
                    }
                    _1435 = (((_1422 * _1421.w) / vec3(_1425)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            float _1437 = 1.0 + (_1306 * _1306);
            _1447 = vec3(_1342.x ? vec3(0.0).x : _1341.x, _1342.y ? vec3(0.0).y : _1341.y, _1342.z ? vec3(0.0).z : _1341.z);
            _1448 = (_1422 * (0.0596831031143665313720703125 * _1437)) + (_1435 * ((0.01627720706164836883544921875 * _1437) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1306), 1.5)));
            break;
        }
    }
    vec3 _1456;
    if (dot(_130, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1456 = _1448 + (_1447 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1456 = _1448;
    }
    vec3 _1474 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1456, _1277, vec3(float(_193))), _801, vec3(float(_213) * min(1.0, 1.0 - smoothstep(0.64999997615814208984375, 0.75, _143))))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    vec4 _1476 = vec4(_1474.x, _1474.y, _1474.z, _109.w);
    _1476.w = 1.0;
    out_var_SV_Target0 = _1476;
    out_var_SV_Target1 = _138;
    gl_FragDepth = _143;
}

