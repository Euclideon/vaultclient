#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

constant float _69 = {};
constant float2 _70 = {};
constant float2 _71 = {};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
    float4 out_var_SV_Target1 [[color(1)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float2 in_var_TEXCOORD2 [[user(locn2)]];
    float2 in_var_TEXCOORD3 [[user(locn3)]];
    float2 in_var_TEXCOORD4 [[user(locn4)]];
    float2 in_var_TEXCOORD5 [[user(locn5)]];
    float in_var_TEXCOORD6 [[user(locn6)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> sceneColourTexture [[texture(0)]], texture2d<float> sceneNormalTexture [[texture(1)]], texture2d<float> sceneDepthTexture [[texture(2)]], sampler sceneColourSampler [[sampler(0)]], sampler sceneNormalSampler [[sampler(1)]], sampler sceneDepthSampler [[sampler(2)]])
{
    main0_out out = {};
    float4 _83 = sceneNormalTexture.sample(sceneNormalSampler, in.in_var_TEXCOORD0);
    float4 _87 = sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD0);
    float _88 = _87.x;
    float4 _542;
    if ((1.0 - (((step(abs(sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD1).x - _88), 0.0030000000260770320892333984375) * step(abs(sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD2).x - _88), 0.0030000000260770320892333984375)) * step(abs(sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD3).x - _88), 0.0030000000260770320892333984375)) * step(abs(sceneDepthTexture.sample(sceneDepthSampler, in.in_var_TEXCOORD4).x - _88), 0.0030000000260770320892333984375))) == 0.0)
    {
        _542 = sceneColourTexture.sample(sceneColourSampler, in.in_var_TEXCOORD0);
    }
    else
    {
        float4 _541;
        switch (0u)
        {
            default:
            {
                float2 _130 = _70;
                _130.x = in.in_var_TEXCOORD0.x;
                float2 _132 = _130;
                _132.y = in.in_var_TEXCOORD0.y;
                float4 _134 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0));
                float4 _136 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(0, 1));
                float _137 = _136.y;
                float4 _139 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(1, 0));
                float _140 = _139.y;
                float4 _142 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(0, -1));
                float _143 = _142.y;
                float4 _145 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(-1, 0));
                float _146 = _145.y;
                float _147 = _134.y;
                float _154 = fast::max(fast::max(_143, _146), fast::max(_140, fast::max(_137, _147)));
                float _157 = _154 - fast::min(fast::min(_143, _146), fast::min(_140, fast::min(_137, _147)));
                if (_157 < fast::max(0.0, _154 * 0.125))
                {
                    _541 = _134;
                    break;
                }
                float4 _163 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(-1));
                float _164 = _163.y;
                float4 _166 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(1));
                float _167 = _166.y;
                float4 _169 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(1, -1));
                float _170 = _169.y;
                float4 _172 = sceneColourTexture.sample(sceneColourSampler, _132, level(0.0), int2(-1, 1));
                float _173 = _172.y;
                float _174 = _143 + _137;
                float _175 = _146 + _140;
                float _178 = (-2.0) * _147;
                float _181 = _170 + _167;
                float _187 = _164 + _173;
                bool _207 = (abs(((-2.0) * _146) + _187) + ((abs(_178 + _174) * 2.0) + abs(((-2.0) * _140) + _181))) >= (abs(((-2.0) * _137) + (_173 + _167)) + ((abs(_178 + _175) * 2.0) + abs(((-2.0) * _143) + (_164 + _170))));
                bool _210 = !_207;
                float _211 = _210 ? _146 : _143;
                float _212 = _210 ? _140 : _137;
                float _216;
                if (_207)
                {
                    _216 = in.in_var_TEXCOORD5.y;
                }
                else
                {
                    _216 = in.in_var_TEXCOORD5.x;
                }
                float _223 = abs(_211 - _147);
                float _224 = abs(_212 - _147);
                bool _225 = _223 >= _224;
                float _230;
                if (_225)
                {
                    _230 = -_216;
                }
                else
                {
                    _230 = _216;
                }
                float _233 = fast::clamp(abs(((((_174 + _175) * 2.0) + (_187 + _181)) * 0.083333335816860198974609375) - _147) * (1.0 / _157), 0.0, 1.0);
                float _234 = _210 ? 0.0 : in.in_var_TEXCOORD5.x;
                float _236 = _207 ? 0.0 : in.in_var_TEXCOORD5.y;
                float2 _242;
                if (_210)
                {
                    float2 _241 = _132;
                    _241.x = in.in_var_TEXCOORD0.x + (_230 * 0.5);
                    _242 = _241;
                }
                else
                {
                    _242 = _132;
                }
                float2 _249;
                if (_207)
                {
                    float2 _248 = _242;
                    _248.y = _242.y + (_230 * 0.5);
                    _249 = _248;
                }
                else
                {
                    _249 = _242;
                }
                float _251 = _249.x - _234;
                float2 _252 = _70;
                _252.x = _251;
                float2 _255 = _252;
                _255.y = _249.y - _236;
                float _256 = _249.x + _234;
                float2 _257 = _70;
                _257.x = _256;
                float2 _259 = _257;
                _259.y = _249.y + _236;
                float _271 = fast::max(_223, _224) * 0.25;
                float _272 = ((!_225) ? (_212 + _147) : (_211 + _147)) * 0.5;
                float _274 = (((-2.0) * _233) + 3.0) * (_233 * _233);
                bool _275 = (_147 - _272) < 0.0;
                float _276 = sceneColourTexture.sample(sceneColourSampler, _255, level(0.0)).y - _272;
                float _277 = sceneColourTexture.sample(sceneColourSampler, _259, level(0.0)).y - _272;
                bool _282 = !(abs(_276) >= _271);
                float2 _288;
                if (_282)
                {
                    float2 _287 = _255;
                    _287.x = _251 - (_234 * 1.5);
                    _288 = _287;
                }
                else
                {
                    _288 = _255;
                }
                float2 _295;
                if (_282)
                {
                    float2 _294 = _288;
                    _294.y = _288.y - (_236 * 1.5);
                    _295 = _294;
                }
                else
                {
                    _295 = _288;
                }
                bool _296 = !(abs(_277) >= _271);
                float2 _303;
                if (_296)
                {
                    float2 _302 = _259;
                    _302.x = _256 + (_234 * 1.5);
                    _303 = _302;
                }
                else
                {
                    _303 = _259;
                }
                float2 _310;
                if (_296)
                {
                    float2 _309 = _303;
                    _309.y = _303.y + (_236 * 1.5);
                    _310 = _309;
                }
                else
                {
                    _310 = _303;
                }
                float2 _489;
                float2 _490;
                float _491;
                float _492;
                if (_282 || _296)
                {
                    float _318;
                    if (_282)
                    {
                        _318 = sceneColourTexture.sample(sceneColourSampler, _295, level(0.0)).y;
                    }
                    else
                    {
                        _318 = _276;
                    }
                    float _324;
                    if (_296)
                    {
                        _324 = sceneColourTexture.sample(sceneColourSampler, _310, level(0.0)).y;
                    }
                    else
                    {
                        _324 = _277;
                    }
                    float _328;
                    if (_282)
                    {
                        _328 = _318 - _272;
                    }
                    else
                    {
                        _328 = _318;
                    }
                    float _332;
                    if (_296)
                    {
                        _332 = _324 - _272;
                    }
                    else
                    {
                        _332 = _324;
                    }
                    bool _337 = !(abs(_328) >= _271);
                    float2 _344;
                    if (_337)
                    {
                        float2 _343 = _295;
                        _343.x = _295.x - (_234 * 2.0);
                        _344 = _343;
                    }
                    else
                    {
                        _344 = _295;
                    }
                    float2 _351;
                    if (_337)
                    {
                        float2 _350 = _344;
                        _350.y = _344.y - (_236 * 2.0);
                        _351 = _350;
                    }
                    else
                    {
                        _351 = _344;
                    }
                    bool _352 = !(abs(_332) >= _271);
                    float2 _360;
                    if (_352)
                    {
                        float2 _359 = _310;
                        _359.x = _310.x + (_234 * 2.0);
                        _360 = _359;
                    }
                    else
                    {
                        _360 = _310;
                    }
                    float2 _367;
                    if (_352)
                    {
                        float2 _366 = _360;
                        _366.y = _360.y + (_236 * 2.0);
                        _367 = _366;
                    }
                    else
                    {
                        _367 = _360;
                    }
                    float2 _485;
                    float2 _486;
                    float _487;
                    float _488;
                    if (_337 || _352)
                    {
                        float _375;
                        if (_337)
                        {
                            _375 = sceneColourTexture.sample(sceneColourSampler, _351, level(0.0)).y;
                        }
                        else
                        {
                            _375 = _328;
                        }
                        float _381;
                        if (_352)
                        {
                            _381 = sceneColourTexture.sample(sceneColourSampler, _367, level(0.0)).y;
                        }
                        else
                        {
                            _381 = _332;
                        }
                        float _385;
                        if (_337)
                        {
                            _385 = _375 - _272;
                        }
                        else
                        {
                            _385 = _375;
                        }
                        float _389;
                        if (_352)
                        {
                            _389 = _381 - _272;
                        }
                        else
                        {
                            _389 = _381;
                        }
                        bool _394 = !(abs(_385) >= _271);
                        float2 _401;
                        if (_394)
                        {
                            float2 _400 = _351;
                            _400.x = _351.x - (_234 * 4.0);
                            _401 = _400;
                        }
                        else
                        {
                            _401 = _351;
                        }
                        float2 _408;
                        if (_394)
                        {
                            float2 _407 = _401;
                            _407.y = _401.y - (_236 * 4.0);
                            _408 = _407;
                        }
                        else
                        {
                            _408 = _401;
                        }
                        bool _409 = !(abs(_389) >= _271);
                        float2 _417;
                        if (_409)
                        {
                            float2 _416 = _367;
                            _416.x = _367.x + (_234 * 4.0);
                            _417 = _416;
                        }
                        else
                        {
                            _417 = _367;
                        }
                        float2 _424;
                        if (_409)
                        {
                            float2 _423 = _417;
                            _423.y = _417.y + (_236 * 4.0);
                            _424 = _423;
                        }
                        else
                        {
                            _424 = _417;
                        }
                        float2 _481;
                        float2 _482;
                        float _483;
                        float _484;
                        if (_394 || _409)
                        {
                            float _432;
                            if (_394)
                            {
                                _432 = sceneColourTexture.sample(sceneColourSampler, _408, level(0.0)).y;
                            }
                            else
                            {
                                _432 = _385;
                            }
                            float _438;
                            if (_409)
                            {
                                _438 = sceneColourTexture.sample(sceneColourSampler, _424, level(0.0)).y;
                            }
                            else
                            {
                                _438 = _389;
                            }
                            float _442;
                            if (_394)
                            {
                                _442 = _432 - _272;
                            }
                            else
                            {
                                _442 = _432;
                            }
                            float _446;
                            if (_409)
                            {
                                _446 = _438 - _272;
                            }
                            else
                            {
                                _446 = _438;
                            }
                            bool _451 = !(abs(_442) >= _271);
                            float2 _458;
                            if (_451)
                            {
                                float2 _457 = _408;
                                _457.x = _408.x - (_234 * 12.0);
                                _458 = _457;
                            }
                            else
                            {
                                _458 = _408;
                            }
                            float2 _465;
                            if (_451)
                            {
                                float2 _464 = _458;
                                _464.y = _458.y - (_236 * 12.0);
                                _465 = _464;
                            }
                            else
                            {
                                _465 = _458;
                            }
                            bool _466 = !(abs(_446) >= _271);
                            float2 _473;
                            if (_466)
                            {
                                float2 _472 = _424;
                                _472.x = _424.x + (_234 * 12.0);
                                _473 = _472;
                            }
                            else
                            {
                                _473 = _424;
                            }
                            float2 _480;
                            if (_466)
                            {
                                float2 _479 = _473;
                                _479.y = _473.y + (_236 * 12.0);
                                _480 = _479;
                            }
                            else
                            {
                                _480 = _473;
                            }
                            _481 = _480;
                            _482 = _465;
                            _483 = _446;
                            _484 = _442;
                        }
                        else
                        {
                            _481 = _424;
                            _482 = _408;
                            _483 = _389;
                            _484 = _385;
                        }
                        _485 = _481;
                        _486 = _482;
                        _487 = _483;
                        _488 = _484;
                    }
                    else
                    {
                        _485 = _367;
                        _486 = _351;
                        _487 = _332;
                        _488 = _328;
                    }
                    _489 = _485;
                    _490 = _486;
                    _491 = _487;
                    _492 = _488;
                }
                else
                {
                    _489 = _310;
                    _490 = _295;
                    _491 = _277;
                    _492 = _276;
                }
                float _501;
                if (_210)
                {
                    _501 = in.in_var_TEXCOORD0.y - _490.y;
                }
                else
                {
                    _501 = in.in_var_TEXCOORD0.x - _490.x;
                }
                float _506;
                if (_210)
                {
                    _506 = _489.y - in.in_var_TEXCOORD0.y;
                }
                else
                {
                    _506 = _489.x - in.in_var_TEXCOORD0.x;
                }
                float _521 = fast::max(((_501 < _506) ? ((_492 < 0.0) != _275) : ((_491 < 0.0) != _275)) ? ((fast::min(_501, _506) * ((-1.0) / (_506 + _501))) + 0.5) : 0.0, (_274 * _274) * 0.75);
                float2 _527;
                if (_210)
                {
                    float2 _526 = _132;
                    _526.x = in.in_var_TEXCOORD0.x + (_521 * _230);
                    _527 = _526;
                }
                else
                {
                    _527 = _132;
                }
                float2 _534;
                if (_207)
                {
                    float2 _533 = _527;
                    _533.y = _527.y + (_521 * _230);
                    _534 = _533;
                }
                else
                {
                    _534 = _527;
                }
                _541 = float4(sceneColourTexture.sample(sceneColourSampler, _534, level(0.0)).xyz, _69);
                break;
            }
        }
        _542 = _541;
    }
    out.out_var_SV_Target0 = float4(mix(float3(dot(_542.xyz, float3(0.2125000059604644775390625, 0.7153999805450439453125, 0.07209999859333038330078125))), _542.xyz, float3(in.in_var_TEXCOORD6)), 1.0);
    out.out_var_SV_Target1 = _83;
    return out;
}

