#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D SPIRV_Cross_Combinedtexture1sampler1;
uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;

in highp vec2 in_var_TEXCOORD0;
in highp vec2 in_var_TEXCOORD1;
in highp vec2 in_var_TEXCOORD2;
in highp vec2 in_var_TEXCOORD3;
in highp vec2 in_var_TEXCOORD4;
in highp vec2 in_var_TEXCOORD5;
in highp float in_var_TEXCOORD6;
layout(location = 0) out highp vec4 out_var_SV_Target;

float _66;
vec2 _67;
vec2 _68;

void main()
{
    highp vec4 _80 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD0);
    highp float _81 = _80.x;
    highp vec4 _535;
    if ((1.0 - (((step(abs(texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD1).x - _81), 0.0030000000260770320892333984375) * step(abs(texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD2).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD3).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD4).x - _81), 0.0030000000260770320892333984375))) == 0.0)
    {
        _535 = texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0);
    }
    else
    {
        highp vec4 _534;
        switch (0u)
        {
            default:
            {
                highp vec2 _123 = _67;
                _123.x = in_var_TEXCOORD0.x;
                highp vec2 _125 = _123;
                _125.y = in_var_TEXCOORD0.y;
                highp vec4 _127 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0);
                highp vec4 _129 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(0, 1));
                highp float _130 = _129.y;
                highp vec4 _132 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(1, 0));
                highp float _133 = _132.y;
                highp vec4 _135 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(0, -1));
                highp float _136 = _135.y;
                highp vec4 _138 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(-1, 0));
                highp float _139 = _138.y;
                highp float _140 = _127.y;
                highp float _147 = max(max(_136, _139), max(_133, max(_130, _140)));
                highp float _150 = _147 - min(min(_136, _139), min(_133, min(_130, _140)));
                if (_150 < max(0.0, _147 * 0.125))
                {
                    _534 = _127;
                    break;
                }
                highp vec4 _156 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(-1));
                highp float _157 = _156.y;
                highp vec4 _159 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(1));
                highp float _160 = _159.y;
                highp vec4 _162 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(1, -1));
                highp float _163 = _162.y;
                highp vec4 _165 = textureLodOffset(SPIRV_Cross_Combinedtexture0sampler0, _125, 0.0, ivec2(-1, 1));
                highp float _166 = _165.y;
                highp float _167 = _136 + _130;
                highp float _168 = _139 + _133;
                highp float _171 = (-2.0) * _140;
                highp float _174 = _163 + _160;
                highp float _180 = _157 + _166;
                bool _200 = (abs(((-2.0) * _139) + _180) + ((abs(_171 + _167) * 2.0) + abs(((-2.0) * _133) + _174))) >= (abs(((-2.0) * _130) + (_166 + _160)) + ((abs(_171 + _168) * 2.0) + abs(((-2.0) * _136) + (_157 + _163))));
                bool _203 = !_200;
                highp float _204 = _203 ? _139 : _136;
                highp float _205 = _203 ? _133 : _130;
                highp float _209;
                if (_200)
                {
                    _209 = in_var_TEXCOORD5.y;
                }
                else
                {
                    _209 = in_var_TEXCOORD5.x;
                }
                highp float _216 = abs(_204 - _140);
                highp float _217 = abs(_205 - _140);
                bool _218 = _216 >= _217;
                highp float _223;
                if (_218)
                {
                    _223 = -_209;
                }
                else
                {
                    _223 = _209;
                }
                highp float _226 = clamp(abs(((((_167 + _168) * 2.0) + (_180 + _174)) * 0.083333335816860198974609375) - _140) * (1.0 / _150), 0.0, 1.0);
                highp float _227 = _203 ? 0.0 : in_var_TEXCOORD5.x;
                highp float _229 = _200 ? 0.0 : in_var_TEXCOORD5.y;
                highp vec2 _235;
                if (_203)
                {
                    highp vec2 _234 = _125;
                    _234.x = in_var_TEXCOORD0.x + (_223 * 0.5);
                    _235 = _234;
                }
                else
                {
                    _235 = _125;
                }
                highp vec2 _242;
                if (_200)
                {
                    highp vec2 _241 = _235;
                    _241.y = _235.y + (_223 * 0.5);
                    _242 = _241;
                }
                else
                {
                    _242 = _235;
                }
                highp float _244 = _242.x - _227;
                highp vec2 _245 = _67;
                _245.x = _244;
                highp vec2 _248 = _245;
                _248.y = _242.y - _229;
                highp float _249 = _242.x + _227;
                highp vec2 _250 = _67;
                _250.x = _249;
                highp vec2 _252 = _250;
                _252.y = _242.y + _229;
                highp float _264 = max(_216, _217) * 0.25;
                highp float _265 = ((!_218) ? (_205 + _140) : (_204 + _140)) * 0.5;
                highp float _267 = (((-2.0) * _226) + 3.0) * (_226 * _226);
                bool _268 = (_140 - _265) < 0.0;
                highp float _269 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _248, 0.0).y - _265;
                highp float _270 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _252, 0.0).y - _265;
                bool _275 = !(abs(_269) >= _264);
                highp vec2 _281;
                if (_275)
                {
                    highp vec2 _280 = _248;
                    _280.x = _244 - (_227 * 1.5);
                    _281 = _280;
                }
                else
                {
                    _281 = _248;
                }
                highp vec2 _288;
                if (_275)
                {
                    highp vec2 _287 = _281;
                    _287.y = _281.y - (_229 * 1.5);
                    _288 = _287;
                }
                else
                {
                    _288 = _281;
                }
                bool _289 = !(abs(_270) >= _264);
                highp vec2 _296;
                if (_289)
                {
                    highp vec2 _295 = _252;
                    _295.x = _249 + (_227 * 1.5);
                    _296 = _295;
                }
                else
                {
                    _296 = _252;
                }
                highp vec2 _303;
                if (_289)
                {
                    highp vec2 _302 = _296;
                    _302.y = _296.y + (_229 * 1.5);
                    _303 = _302;
                }
                else
                {
                    _303 = _296;
                }
                highp vec2 _482;
                highp vec2 _483;
                highp float _484;
                highp float _485;
                if (_275 || _289)
                {
                    highp float _311;
                    if (_275)
                    {
                        _311 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _288, 0.0).y;
                    }
                    else
                    {
                        _311 = _269;
                    }
                    highp float _317;
                    if (_289)
                    {
                        _317 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _303, 0.0).y;
                    }
                    else
                    {
                        _317 = _270;
                    }
                    highp float _321;
                    if (_275)
                    {
                        _321 = _311 - _265;
                    }
                    else
                    {
                        _321 = _311;
                    }
                    highp float _325;
                    if (_289)
                    {
                        _325 = _317 - _265;
                    }
                    else
                    {
                        _325 = _317;
                    }
                    bool _330 = !(abs(_321) >= _264);
                    highp vec2 _337;
                    if (_330)
                    {
                        highp vec2 _336 = _288;
                        _336.x = _288.x - (_227 * 2.0);
                        _337 = _336;
                    }
                    else
                    {
                        _337 = _288;
                    }
                    highp vec2 _344;
                    if (_330)
                    {
                        highp vec2 _343 = _337;
                        _343.y = _337.y - (_229 * 2.0);
                        _344 = _343;
                    }
                    else
                    {
                        _344 = _337;
                    }
                    bool _345 = !(abs(_325) >= _264);
                    highp vec2 _353;
                    if (_345)
                    {
                        highp vec2 _352 = _303;
                        _352.x = _303.x + (_227 * 2.0);
                        _353 = _352;
                    }
                    else
                    {
                        _353 = _303;
                    }
                    highp vec2 _360;
                    if (_345)
                    {
                        highp vec2 _359 = _353;
                        _359.y = _353.y + (_229 * 2.0);
                        _360 = _359;
                    }
                    else
                    {
                        _360 = _353;
                    }
                    highp vec2 _478;
                    highp vec2 _479;
                    highp float _480;
                    highp float _481;
                    if (_330 || _345)
                    {
                        highp float _368;
                        if (_330)
                        {
                            _368 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _344, 0.0).y;
                        }
                        else
                        {
                            _368 = _321;
                        }
                        highp float _374;
                        if (_345)
                        {
                            _374 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _360, 0.0).y;
                        }
                        else
                        {
                            _374 = _325;
                        }
                        highp float _378;
                        if (_330)
                        {
                            _378 = _368 - _265;
                        }
                        else
                        {
                            _378 = _368;
                        }
                        highp float _382;
                        if (_345)
                        {
                            _382 = _374 - _265;
                        }
                        else
                        {
                            _382 = _374;
                        }
                        bool _387 = !(abs(_378) >= _264);
                        highp vec2 _394;
                        if (_387)
                        {
                            highp vec2 _393 = _344;
                            _393.x = _344.x - (_227 * 4.0);
                            _394 = _393;
                        }
                        else
                        {
                            _394 = _344;
                        }
                        highp vec2 _401;
                        if (_387)
                        {
                            highp vec2 _400 = _394;
                            _400.y = _394.y - (_229 * 4.0);
                            _401 = _400;
                        }
                        else
                        {
                            _401 = _394;
                        }
                        bool _402 = !(abs(_382) >= _264);
                        highp vec2 _410;
                        if (_402)
                        {
                            highp vec2 _409 = _360;
                            _409.x = _360.x + (_227 * 4.0);
                            _410 = _409;
                        }
                        else
                        {
                            _410 = _360;
                        }
                        highp vec2 _417;
                        if (_402)
                        {
                            highp vec2 _416 = _410;
                            _416.y = _410.y + (_229 * 4.0);
                            _417 = _416;
                        }
                        else
                        {
                            _417 = _410;
                        }
                        highp vec2 _474;
                        highp vec2 _475;
                        highp float _476;
                        highp float _477;
                        if (_387 || _402)
                        {
                            highp float _425;
                            if (_387)
                            {
                                _425 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _401, 0.0).y;
                            }
                            else
                            {
                                _425 = _378;
                            }
                            highp float _431;
                            if (_402)
                            {
                                _431 = textureLod(SPIRV_Cross_Combinedtexture0sampler0, _417, 0.0).y;
                            }
                            else
                            {
                                _431 = _382;
                            }
                            highp float _435;
                            if (_387)
                            {
                                _435 = _425 - _265;
                            }
                            else
                            {
                                _435 = _425;
                            }
                            highp float _439;
                            if (_402)
                            {
                                _439 = _431 - _265;
                            }
                            else
                            {
                                _439 = _431;
                            }
                            bool _444 = !(abs(_435) >= _264);
                            highp vec2 _451;
                            if (_444)
                            {
                                highp vec2 _450 = _401;
                                _450.x = _401.x - (_227 * 12.0);
                                _451 = _450;
                            }
                            else
                            {
                                _451 = _401;
                            }
                            highp vec2 _458;
                            if (_444)
                            {
                                highp vec2 _457 = _451;
                                _457.y = _451.y - (_229 * 12.0);
                                _458 = _457;
                            }
                            else
                            {
                                _458 = _451;
                            }
                            bool _459 = !(abs(_439) >= _264);
                            highp vec2 _466;
                            if (_459)
                            {
                                highp vec2 _465 = _417;
                                _465.x = _417.x + (_227 * 12.0);
                                _466 = _465;
                            }
                            else
                            {
                                _466 = _417;
                            }
                            highp vec2 _473;
                            if (_459)
                            {
                                highp vec2 _472 = _466;
                                _472.y = _466.y + (_229 * 12.0);
                                _473 = _472;
                            }
                            else
                            {
                                _473 = _466;
                            }
                            _474 = _473;
                            _475 = _458;
                            _476 = _439;
                            _477 = _435;
                        }
                        else
                        {
                            _474 = _417;
                            _475 = _401;
                            _476 = _382;
                            _477 = _378;
                        }
                        _478 = _474;
                        _479 = _475;
                        _480 = _476;
                        _481 = _477;
                    }
                    else
                    {
                        _478 = _360;
                        _479 = _344;
                        _480 = _325;
                        _481 = _321;
                    }
                    _482 = _478;
                    _483 = _479;
                    _484 = _480;
                    _485 = _481;
                }
                else
                {
                    _482 = _303;
                    _483 = _288;
                    _484 = _270;
                    _485 = _269;
                }
                highp float _494;
                if (_203)
                {
                    _494 = in_var_TEXCOORD0.y - _483.y;
                }
                else
                {
                    _494 = in_var_TEXCOORD0.x - _483.x;
                }
                highp float _499;
                if (_203)
                {
                    _499 = _482.y - in_var_TEXCOORD0.y;
                }
                else
                {
                    _499 = _482.x - in_var_TEXCOORD0.x;
                }
                highp float _514 = max(((_494 < _499) ? ((_485 < 0.0) != _268) : ((_484 < 0.0) != _268)) ? ((min(_494, _499) * ((-1.0) / (_499 + _494))) + 0.5) : 0.0, (_267 * _267) * 0.75);
                highp vec2 _520;
                if (_203)
                {
                    highp vec2 _519 = _125;
                    _519.x = in_var_TEXCOORD0.x + (_514 * _223);
                    _520 = _519;
                }
                else
                {
                    _520 = _125;
                }
                highp vec2 _527;
                if (_200)
                {
                    highp vec2 _526 = _520;
                    _526.y = _520.y + (_514 * _223);
                    _527 = _526;
                }
                else
                {
                    _527 = _520;
                }
                _534 = vec4(textureLod(SPIRV_Cross_Combinedtexture0sampler0, _527, 0.0).xyz, _66);
                break;
            }
        }
        _535 = _534;
    }
    out_var_SV_Target = vec4(mix(vec3(dot(_535.xyz, vec3(0.2125000059604644775390625, 0.7153999805450439453125, 0.07209999859333038330078125))), _535.xyz, vec3(in_var_TEXCOORD6)), 1.0);
}

