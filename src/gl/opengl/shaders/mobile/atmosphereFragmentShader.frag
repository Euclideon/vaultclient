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

uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform highp sampler2D SPIRV_Cross_CombinedirradianceTextureirradianceSampler;
uniform highp sampler2D SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler;
uniform highp sampler3D SPIRV_Cross_CombinedscatteringTexturescatteringSampler;

in highp vec2 varying_TEXCOORD0;
in highp vec3 varying_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

vec4 _105;

void main()
{
    highp float _113 = u_fragParams.u_earthCenter.w + 60000.0;
    highp vec3 _126 = normalize(varying_TEXCOORD1);
    highp vec4 _130 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD0);
    highp float _131 = _130.x;
    highp float _136 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    highp vec4 _151 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD0);
    highp vec3 _154 = pow(abs(_151.xyz), vec3(2.2000000476837158203125));
    highp float _160 = ((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (((u_cameraPlaneParams.s_CameraFarPlane / _136) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _131 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * _136))) * u_cameraPlaneParams.s_CameraFarPlane;
    bool _163 = _131 < 0.64999997615814208984375;
    highp vec3 _754;
    if (_163)
    {
        highp vec3 _166 = (u_fragParams.u_camera.xyz + (_126 * _160)) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _167 = normalize(_166);
        highp float _170 = length(_166);
        highp float _172 = dot(_166, u_fragParams.u_sunDirection.xyz) / _170;
        highp float _174 = _113 - u_fragParams.u_earthCenter.w;
        highp vec4 _185 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_172 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_170 - u_fragParams.u_earthCenter.w) / _174) * 0.9375)));
        highp float _192 = u_fragParams.u_earthCenter.w / _170;
        highp float _198 = _113 * _113;
        highp float _199 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
        highp float _201 = sqrt(_198 - _199);
        highp float _202 = _170 * _170;
        highp float _205 = sqrt(max(_202 - _199, 0.0));
        highp float _216 = _113 - _170;
        highp vec4 _229 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_170) * _172) + sqrt(max((_202 * ((_172 * _172) - 1.0)) + _198, 0.0)), 0.0) - _216) / ((_205 + _201) - _216)) * 0.99609375), 0.0078125 + ((_205 / _201) * 0.984375)));
        highp float _248 = mix(_160 * 0.64999997615814208984375, 0.0, pow(_131 * 1.53846156597137451171875, 8.0));
        highp vec3 _249 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
        highp vec3 _252 = normalize(_166 - _249);
        highp float _253 = length(_249);
        highp float _254 = dot(_249, _252);
        highp float _261 = (-_254) - sqrt(((_254 * _254) - (_253 * _253)) + _198);
        bool _262 = _261 > 0.0;
        highp vec3 _268;
        highp float _269;
        if (_262)
        {
            _268 = _249 + (_252 * _261);
            _269 = _254 + _261;
        }
        else
        {
            _268 = _249;
            _269 = _254;
        }
        highp float _288;
        highp float _270 = _262 ? _113 : _253;
        highp float _271 = _269 / _270;
        highp float _272 = dot(_268, u_fragParams.u_sunDirection.xyz);
        highp float _273 = _272 / _270;
        highp float _274 = dot(_252, u_fragParams.u_sunDirection.xyz);
        highp float _276 = length(_166 - _268);
        highp float _278 = _270 * _270;
        highp float _281 = _278 * ((_271 * _271) - 1.0);
        bool _284 = (_271 < 0.0) && ((_281 + _199) >= 0.0);
        highp vec3 _413;
        switch (0u)
        {
            default:
            {
                _288 = (2.0 * _270) * _271;
                highp float _293 = clamp(sqrt((_276 * (_276 + _288)) + _278), u_fragParams.u_earthCenter.w, _113);
                highp float _296 = clamp((_269 + _276) / _293, -1.0, 1.0);
                if (_284)
                {
                    highp float _354 = -_296;
                    highp float _355 = _293 * _293;
                    highp float _358 = sqrt(max(_355 - _199, 0.0));
                    highp float _369 = _113 - _293;
                    highp float _383 = -_271;
                    highp float _386 = sqrt(max(_278 - _199, 0.0));
                    highp float _397 = _113 - _270;
                    _413 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_293) * _354) + sqrt(max((_355 * ((_354 * _354) - 1.0)) + _198, 0.0)), 0.0) - _369) / ((_358 + _201) - _369)) * 0.99609375), 0.0078125 + ((_358 / _201) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_270) * _383) + sqrt(max((_278 * ((_383 * _383) - 1.0)) + _198, 0.0)), 0.0) - _397) / ((_386 + _201) - _397)) * 0.99609375), 0.0078125 + ((_386 / _201) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _302 = sqrt(max(_278 - _199, 0.0));
                    highp float _310 = _113 - _270;
                    highp float _324 = _293 * _293;
                    highp float _327 = sqrt(max(_324 - _199, 0.0));
                    highp float _338 = _113 - _293;
                    _413 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_270) * _271) + sqrt(max(_281 + _198, 0.0)), 0.0) - _310) / ((_302 + _201) - _310)) * 0.99609375), 0.0078125 + ((_302 / _201) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_293) * _296) + sqrt(max((_324 * ((_296 * _296) - 1.0)) + _198, 0.0)), 0.0) - _338) / ((_327 + _201) - _338)) * 0.99609375), 0.0078125 + ((_327 / _201) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _416 = sqrt(max(_278 - _199, 0.0));
        highp float _417 = _416 / _201;
        highp float _419 = 0.015625 + (_417 * 0.96875);
        highp float _422 = ((_269 * _269) - _278) + _199;
        highp float _455;
        if (_284)
        {
            highp float _445 = _270 - u_fragParams.u_earthCenter.w;
            _455 = 0.5 - (0.5 * (0.0078125 + (((_416 == _445) ? 0.0 : ((((-_269) - sqrt(max(_422, 0.0))) - _445) / (_416 - _445))) * 0.984375)));
        }
        else
        {
            highp float _432 = _113 - _270;
            _455 = 0.5 + (0.5 * (0.0078125 + (((((-_269) + sqrt(max(_422 + (_201 * _201), 0.0))) - _432) / ((_416 + _201) - _432)) * 0.984375)));
        }
        highp float _460 = -u_fragParams.u_earthCenter.w;
        highp float _467 = _201 - _174;
        highp float _468 = (max((_460 * _273) + sqrt(max((_199 * ((_273 * _273) - 1.0)) + _198, 0.0)), 0.0) - _174) / _467;
        highp float _470 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _467;
        highp float _477 = 0.015625 + ((max(1.0 - (_468 / _470), 0.0) / (1.0 + _468)) * 0.96875);
        highp float _479 = (_274 + 1.0) * 3.5;
        highp float _480 = floor(_479);
        highp float _481 = _479 - _480;
        highp float _485 = _480 + 1.0;
        highp vec4 _491 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_480 + _477) * 0.125, _455, _419));
        highp float _492 = 1.0 - _481;
        highp vec4 _495 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_485 + _477) * 0.125, _455, _419));
        highp vec4 _497 = (_491 * _492) + (_495 * _481);
        highp vec3 _498 = _497.xyz;
        highp vec3 _511;
        switch (0u)
        {
            default:
            {
                highp float _501 = _497.x;
                if (_501 == 0.0)
                {
                    _511 = vec3(0.0);
                    break;
                }
                _511 = (((_498 * _497.w) / vec3(_501)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _513 = max(_276 - _248, 0.0);
        highp float _518 = clamp(sqrt((_513 * (_513 + _288)) + _278), u_fragParams.u_earthCenter.w, _113);
        highp float _519 = _269 + _513;
        highp float _522 = (_272 + (_513 * _274)) / _518;
        highp float _523 = _518 * _518;
        highp float _526 = sqrt(max(_523 - _199, 0.0));
        highp float _527 = _526 / _201;
        highp float _529 = 0.015625 + (_527 * 0.96875);
        highp float _532 = ((_519 * _519) - _523) + _199;
        highp float _565;
        if (_284)
        {
            highp float _555 = _518 - u_fragParams.u_earthCenter.w;
            _565 = 0.5 - (0.5 * (0.0078125 + (((_526 == _555) ? 0.0 : ((((-_519) - sqrt(max(_532, 0.0))) - _555) / (_526 - _555))) * 0.984375)));
        }
        else
        {
            highp float _542 = _113 - _518;
            _565 = 0.5 + (0.5 * (0.0078125 + (((((-_519) + sqrt(max(_532 + (_201 * _201), 0.0))) - _542) / ((_526 + _201) - _542)) * 0.984375)));
        }
        highp float _576 = (max((_460 * _522) + sqrt(max((_199 * ((_522 * _522) - 1.0)) + _198, 0.0)), 0.0) - _174) / _467;
        highp float _583 = 0.015625 + ((max(1.0 - (_576 / _470), 0.0) / (1.0 + _576)) * 0.96875);
        highp vec4 _591 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_480 + _583) * 0.125, _565, _529));
        highp vec4 _594 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_485 + _583) * 0.125, _565, _529));
        highp vec4 _596 = (_591 * _492) + (_594 * _481);
        highp vec3 _597 = _596.xyz;
        highp vec3 _610;
        switch (0u)
        {
            default:
            {
                highp float _600 = _596.x;
                if (_600 == 0.0)
                {
                    _610 = vec3(0.0);
                    break;
                }
                _610 = (((_597 * _596.w) / vec3(_600)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _717;
        if (_248 > 0.0)
        {
            highp vec3 _716;
            switch (0u)
            {
                default:
                {
                    highp float _617 = clamp(_519 / _518, -1.0, 1.0);
                    if (_284)
                    {
                        highp float _666 = -_617;
                        highp float _677 = _113 - _518;
                        highp float _690 = -_271;
                        highp float _701 = _113 - _270;
                        _716 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_518) * _666) + sqrt(max((_523 * ((_666 * _666) - 1.0)) + _198, 0.0)), 0.0) - _677) / ((_526 + _201) - _677)) * 0.99609375), 0.0078125 + (_527 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_270) * _690) + sqrt(max((_278 * ((_690 * _690) - 1.0)) + _198, 0.0)), 0.0) - _701) / ((_416 + _201) - _701)) * 0.99609375), 0.0078125 + (_417 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                    else
                    {
                        highp float _628 = _113 - _270;
                        highp float _651 = _113 - _518;
                        _716 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_270) * _271) + sqrt(max(_281 + _198, 0.0)), 0.0) - _628) / ((_416 + _201) - _628)) * 0.99609375), 0.0078125 + (_417 * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_518) * _617) + sqrt(max((_523 * ((_617 * _617) - 1.0)) + _198, 0.0)), 0.0) - _651) / ((_526 + _201) - _651)) * 0.99609375), 0.0078125 + (_527 * 0.984375))).xyz, vec3(1.0));
                        break;
                    }
                }
            }
            _717 = _716;
        }
        else
        {
            _717 = _413;
        }
        highp vec3 _719 = _498 - (_717 * _597);
        highp vec3 _721 = _511 - (_717 * _610);
        highp float _722 = _721.x;
        highp float _723 = _719.x;
        highp vec3 _738;
        switch (0u)
        {
            default:
            {
                if (_723 == 0.0)
                {
                    _738 = vec3(0.0);
                    break;
                }
                _738 = (((vec4(_723, _719.yz, _722).xyz * _722) / vec3(_723)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _742 = 1.0 + (_274 * _274);
        _754 = (((_154.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_229.xyz * smoothstep(_192 * (-0.004674999974668025970458984375), _192 * 0.004674999974668025970458984375, _172 - (-sqrt(max(1.0 - (_192 * _192), 0.0))))) * max(dot(_167, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_185.xyz * (1.0 + (dot(_167, _166) / _170))) * 0.5))) * _413) + ((_719 * (0.0596831031143665313720703125 * _742)) + ((_738 * smoothstep(0.0, 0.00999999977648258209228515625, _273)) * ((0.01627720706164836883544921875 * _742) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _274), 1.5))));
    }
    else
    {
        _754 = vec3(0.0);
    }
    highp vec3 _756 = u_fragParams.u_camera.xyz - u_fragParams.u_earthCenter.xyz;
    highp float _757 = dot(_756, _126);
    highp float _759 = _757 * _757;
    highp float _761 = -_757;
    highp float _762 = u_fragParams.u_earthCenter.w * u_fragParams.u_earthCenter.w;
    highp float _765 = _761 - sqrt(_762 - (dot(_756, _756) - _759));
    bool _766 = _765 > 0.0;
    highp vec3 _1245;
    if (_766)
    {
        highp vec3 _771 = (u_fragParams.u_camera.xyz + (_126 * _765)) - u_fragParams.u_earthCenter.xyz;
        highp vec3 _772 = normalize(_771);
        highp float _775 = length(_771);
        highp float _777 = dot(_771, u_fragParams.u_sunDirection.xyz) / _775;
        highp float _779 = _113 - u_fragParams.u_earthCenter.w;
        highp vec4 _790 = texture(SPIRV_Cross_CombinedirradianceTextureirradianceSampler, vec2(0.0078125 + (((_777 * 0.5) + 0.5) * 0.984375), 0.03125 + (((_775 - u_fragParams.u_earthCenter.w) / _779) * 0.9375)));
        highp float _797 = u_fragParams.u_earthCenter.w / _775;
        highp float _803 = _113 * _113;
        highp float _805 = sqrt(_803 - _762);
        highp float _806 = _775 * _775;
        highp float _809 = sqrt(max(_806 - _762, 0.0));
        highp float _820 = _113 - _775;
        highp vec4 _833 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_775) * _777) + sqrt(max((_806 * ((_777 * _777) - 1.0)) + _803, 0.0)), 0.0) - _820) / ((_809 + _805) - _820)) * 0.99609375), 0.0078125 + ((_809 / _805) * 0.984375)));
        highp vec3 _851 = normalize(_771 - _756);
        highp float _852 = length(_756);
        highp float _853 = dot(_756, _851);
        highp float _860 = (-_853) - sqrt(((_853 * _853) - (_852 * _852)) + _803);
        bool _861 = _860 > 0.0;
        highp vec3 _867;
        highp float _868;
        if (_861)
        {
            _867 = _756 + (_851 * _860);
            _868 = _853 + _860;
        }
        else
        {
            _867 = _756;
            _868 = _853;
        }
        highp float _887;
        highp float _869 = _861 ? _113 : _852;
        highp float _870 = _868 / _869;
        highp float _871 = dot(_867, u_fragParams.u_sunDirection.xyz);
        highp float _872 = _871 / _869;
        highp float _873 = dot(_851, u_fragParams.u_sunDirection.xyz);
        highp float _875 = length(_771 - _867);
        highp float _877 = _869 * _869;
        highp float _880 = _877 * ((_870 * _870) - 1.0);
        bool _883 = (_870 < 0.0) && ((_880 + _762) >= 0.0);
        highp vec3 _1012;
        switch (0u)
        {
            default:
            {
                _887 = (2.0 * _869) * _870;
                highp float _892 = clamp(sqrt((_875 * (_875 + _887)) + _877), u_fragParams.u_earthCenter.w, _113);
                highp float _895 = clamp((_868 + _875) / _892, -1.0, 1.0);
                if (_883)
                {
                    highp float _953 = -_895;
                    highp float _954 = _892 * _892;
                    highp float _957 = sqrt(max(_954 - _762, 0.0));
                    highp float _968 = _113 - _892;
                    highp float _982 = -_870;
                    highp float _985 = sqrt(max(_877 - _762, 0.0));
                    highp float _996 = _113 - _869;
                    _1012 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_892) * _953) + sqrt(max((_954 * ((_953 * _953) - 1.0)) + _803, 0.0)), 0.0) - _968) / ((_957 + _805) - _968)) * 0.99609375), 0.0078125 + ((_957 / _805) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_869) * _982) + sqrt(max((_877 * ((_982 * _982) - 1.0)) + _803, 0.0)), 0.0) - _996) / ((_985 + _805) - _996)) * 0.99609375), 0.0078125 + ((_985 / _805) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
                else
                {
                    highp float _901 = sqrt(max(_877 - _762, 0.0));
                    highp float _909 = _113 - _869;
                    highp float _923 = _892 * _892;
                    highp float _926 = sqrt(max(_923 - _762, 0.0));
                    highp float _937 = _113 - _892;
                    _1012 = min(texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_869) * _870) + sqrt(max(_880 + _803, 0.0)), 0.0) - _909) / ((_901 + _805) - _909)) * 0.99609375), 0.0078125 + ((_901 / _805) * 0.984375))).xyz / texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_892) * _895) + sqrt(max((_923 * ((_895 * _895) - 1.0)) + _803, 0.0)), 0.0) - _937) / ((_926 + _805) - _937)) * 0.99609375), 0.0078125 + ((_926 / _805) * 0.984375))).xyz, vec3(1.0));
                    break;
                }
            }
        }
        highp float _1015 = sqrt(max(_877 - _762, 0.0));
        highp float _1018 = 0.015625 + ((_1015 / _805) * 0.96875);
        highp float _1021 = ((_868 * _868) - _877) + _762;
        highp float _1054;
        if (_883)
        {
            highp float _1044 = _869 - u_fragParams.u_earthCenter.w;
            _1054 = 0.5 - (0.5 * (0.0078125 + (((_1015 == _1044) ? 0.0 : ((((-_868) - sqrt(max(_1021, 0.0))) - _1044) / (_1015 - _1044))) * 0.984375)));
        }
        else
        {
            highp float _1031 = _113 - _869;
            _1054 = 0.5 + (0.5 * (0.0078125 + (((((-_868) + sqrt(max(_1021 + (_805 * _805), 0.0))) - _1031) / ((_1015 + _805) - _1031)) * 0.984375)));
        }
        highp float _1059 = -u_fragParams.u_earthCenter.w;
        highp float _1066 = _805 - _779;
        highp float _1067 = (max((_1059 * _872) + sqrt(max((_762 * ((_872 * _872) - 1.0)) + _803, 0.0)), 0.0) - _779) / _1066;
        highp float _1069 = (0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1066;
        highp float _1076 = 0.015625 + ((max(1.0 - (_1067 / _1069), 0.0) / (1.0 + _1067)) * 0.96875);
        highp float _1078 = (_873 + 1.0) * 3.5;
        highp float _1079 = floor(_1078);
        highp float _1080 = _1078 - _1079;
        highp float _1084 = _1079 + 1.0;
        highp vec4 _1090 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1079 + _1076) * 0.125, _1054, _1018));
        highp float _1091 = 1.0 - _1080;
        highp vec4 _1094 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1084 + _1076) * 0.125, _1054, _1018));
        highp vec4 _1096 = (_1090 * _1091) + (_1094 * _1080);
        highp vec3 _1097 = _1096.xyz;
        highp vec3 _1110;
        switch (0u)
        {
            default:
            {
                highp float _1100 = _1096.x;
                if (_1100 == 0.0)
                {
                    _1110 = vec3(0.0);
                    break;
                }
                _1110 = (((_1097 * _1096.w) / vec3(_1100)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1111 = max(_875, 0.0);
        highp float _1116 = clamp(sqrt((_1111 * (_1111 + _887)) + _877), u_fragParams.u_earthCenter.w, _113);
        highp float _1117 = _868 + _1111;
        highp float _1120 = (_871 + (_1111 * _873)) / _1116;
        highp float _1121 = _1116 * _1116;
        highp float _1124 = sqrt(max(_1121 - _762, 0.0));
        highp float _1127 = 0.015625 + ((_1124 / _805) * 0.96875);
        highp float _1130 = ((_1117 * _1117) - _1121) + _762;
        highp float _1163;
        if (_883)
        {
            highp float _1153 = _1116 - u_fragParams.u_earthCenter.w;
            _1163 = 0.5 - (0.5 * (0.0078125 + (((_1124 == _1153) ? 0.0 : ((((-_1117) - sqrt(max(_1130, 0.0))) - _1153) / (_1124 - _1153))) * 0.984375)));
        }
        else
        {
            highp float _1140 = _113 - _1116;
            _1163 = 0.5 + (0.5 * (0.0078125 + (((((-_1117) + sqrt(max(_1130 + (_805 * _805), 0.0))) - _1140) / ((_1124 + _805) - _1140)) * 0.984375)));
        }
        highp float _1174 = (max((_1059 * _1120) + sqrt(max((_762 * ((_1120 * _1120) - 1.0)) + _803, 0.0)), 0.0) - _779) / _1066;
        highp float _1181 = 0.015625 + ((max(1.0 - (_1174 / _1069), 0.0) / (1.0 + _1174)) * 0.96875);
        highp vec4 _1189 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1079 + _1181) * 0.125, _1163, _1127));
        highp vec4 _1192 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1084 + _1181) * 0.125, _1163, _1127));
        highp vec4 _1194 = (_1189 * _1091) + (_1192 * _1080);
        highp vec3 _1195 = _1194.xyz;
        highp vec3 _1208;
        switch (0u)
        {
            default:
            {
                highp float _1198 = _1194.x;
                if (_1198 == 0.0)
                {
                    _1208 = vec3(0.0);
                    break;
                }
                _1208 = (((_1195 * _1194.w) / vec3(_1198)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp vec3 _1210 = _1097 - (_1012 * _1195);
        highp vec3 _1212 = _1110 - (_1012 * _1208);
        highp float _1213 = _1212.x;
        highp float _1214 = _1210.x;
        highp vec3 _1229;
        switch (0u)
        {
            default:
            {
                if (_1214 == 0.0)
                {
                    _1229 = vec3(0.0);
                    break;
                }
                _1229 = (((vec4(_1214, _1210.yz, _1213).xyz * _1213) / vec3(_1214)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                break;
            }
        }
        highp float _1233 = 1.0 + (_873 * _873);
        _1245 = (((_154.xyz * 0.3183098733425140380859375) * ((vec3(1.47399997711181640625, 1.85039997100830078125, 1.91198003292083740234375) * max((_833.xyz * smoothstep(_797 * (-0.004674999974668025970458984375), _797 * 0.004674999974668025970458984375, _777 - (-sqrt(max(1.0 - (_797 * _797), 0.0))))) * max(dot(_772, u_fragParams.u_sunDirection.xyz), 0.0), vec3(0.001000000047497451305389404296875))) + ((_790.xyz * (1.0 + (dot(_772, _771) / _775))) * 0.5))) * _1012) + ((_1210 * (0.0596831031143665313720703125 * _1233)) + ((_1229 * smoothstep(0.0, 0.00999999977648258209228515625, _872)) * ((0.01627720706164836883544921875 * _1233) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _873), 1.5))));
    }
    else
    {
        _1245 = vec3(0.0);
    }
    highp vec3 _1415;
    highp vec3 _1416;
    switch (0u)
    {
        default:
        {
            highp float _1251 = length(_756);
            highp float _1254 = _113 * _113;
            highp float _1257 = _761 - sqrt((_759 - (_1251 * _1251)) + _1254);
            bool _1258 = _1257 > 0.0;
            highp vec3 _1268;
            highp float _1269;
            if (_1258)
            {
                _1268 = _756 + (_126 * _1257);
                _1269 = _757 + _1257;
            }
            else
            {
                if (_1251 > _113)
                {
                    _1415 = vec3(1.0);
                    _1416 = vec3(0.0);
                    break;
                }
                _1268 = _756;
                _1269 = _757;
            }
            highp float _1270 = _1258 ? _113 : _1251;
            highp float _1271 = _1269 / _1270;
            highp float _1273 = dot(_1268, u_fragParams.u_sunDirection.xyz) / _1270;
            highp float _1274 = dot(_126, u_fragParams.u_sunDirection.xyz);
            highp float _1276 = _1270 * _1270;
            highp float _1279 = _1276 * ((_1271 * _1271) - 1.0);
            bool _1282 = (_1271 < 0.0) && ((_1279 + _762) >= 0.0);
            highp float _1284 = sqrt(_1254 - _762);
            highp float _1287 = sqrt(max(_1276 - _762, 0.0));
            highp float _1295 = _113 - _1270;
            highp float _1298 = (_1287 + _1284) - _1295;
            highp float _1300 = _1287 / _1284;
            highp vec4 _1308 = texture(SPIRV_Cross_CombinedtransmittanceTexturetransmittanceSampler, vec2(0.001953125 + (((max(((-_1270) * _1271) + sqrt(max(_1279 + _1254, 0.0)), 0.0) - _1295) / _1298) * 0.99609375), 0.0078125 + (_1300 * 0.984375)));
            highp vec3 _1309 = _1308.xyz;
            bvec3 _1310 = bvec3(_1282);
            highp float _1313 = 0.015625 + (_1300 * 0.96875);
            highp float _1316 = ((_1269 * _1269) - _1276) + _762;
            highp float _1346;
            if (_1282)
            {
                highp float _1336 = _1270 - u_fragParams.u_earthCenter.w;
                _1346 = 0.5 - (0.5 * (0.0078125 + (((_1287 == _1336) ? 0.0 : ((((-_1269) - sqrt(max(_1316, 0.0))) - _1336) / (_1287 - _1336))) * 0.984375)));
            }
            else
            {
                _1346 = 0.5 + (0.5 * (0.0078125 + (((((-_1269) + sqrt(max(_1316 + (_1284 * _1284), 0.0))) - _1295) / _1298) * 0.984375)));
            }
            highp float _1357 = _113 - u_fragParams.u_earthCenter.w;
            highp float _1359 = _1284 - _1357;
            highp float _1360 = (max(((-u_fragParams.u_earthCenter.w) * _1273) + sqrt(max((_762 * ((_1273 * _1273) - 1.0)) + _1254, 0.0)), 0.0) - _1357) / _1359;
            highp float _1369 = 0.015625 + ((max(1.0 - (_1360 / ((0.415823996067047119140625 * u_fragParams.u_earthCenter.w) / _1359)), 0.0) / (1.0 + _1360)) * 0.96875);
            highp float _1371 = (_1274 + 1.0) * 3.5;
            highp float _1372 = floor(_1371);
            highp float _1373 = _1371 - _1372;
            highp vec4 _1383 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3((_1372 + _1369) * 0.125, _1346, _1313));
            highp vec4 _1387 = texture(SPIRV_Cross_CombinedscatteringTexturescatteringSampler, vec3(((_1372 + 1.0) + _1369) * 0.125, _1346, _1313));
            highp vec4 _1389 = (_1383 * (1.0 - _1373)) + (_1387 * _1373);
            highp vec3 _1390 = _1389.xyz;
            highp vec3 _1403;
            switch (0u)
            {
                default:
                {
                    highp float _1393 = _1389.x;
                    if (_1393 == 0.0)
                    {
                        _1403 = vec3(0.0);
                        break;
                    }
                    _1403 = (((_1390 * _1389.w) / vec3(_1393)) * 1.5) * vec3(0.66666662693023681640625, 0.28571426868438720703125, 0.121212117373943328857421875);
                    break;
                }
            }
            highp float _1405 = 1.0 + (_1274 * _1274);
            _1415 = vec3(_1310.x ? vec3(0.0).x : _1309.x, _1310.y ? vec3(0.0).y : _1309.y, _1310.z ? vec3(0.0).z : _1309.z);
            _1416 = (_1390 * (0.0596831031143665313720703125 * _1405)) + (_1403 * ((0.01627720706164836883544921875 * _1405) / pow(1.6400001049041748046875 - (1.60000002384185791015625 * _1274), 1.5)));
            break;
        }
    }
    highp vec3 _1424;
    if (dot(_126, u_fragParams.u_sunDirection.xyz) > u_fragParams.u_sunSize.y)
    {
        _1424 = _1416 + (_1415 * vec3(21467.642578125, 26949.611328125, 27846.474609375));
    }
    else
    {
        _1424 = _1416;
    }
    highp vec3 _1438 = pow(abs(vec3(1.0) - exp(((-mix(mix(_1424, _1245, vec3(float(_766))), _754, vec3(float(_163)))) / u_fragParams.u_whitePoint.xyz) * u_fragParams.u_camera.w)), vec3(0.4545454680919647216796875));
    highp vec4 _1440 = vec4(_1438.x, _1438.y, _1438.z, _105.w);
    _1440.w = 1.0;
    out_var_SV_Target = _1440;
    gl_FragDepth = _131;
}

