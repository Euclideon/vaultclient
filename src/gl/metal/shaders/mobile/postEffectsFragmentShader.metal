#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

constant float _63 = {};
constant float2 _64 = {};
constant float2 _65 = {};

struct main0_out
{
    float4 out_var_SV_Target0 [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD5 [[user(locn5)]];
    float in_var_TEXCOORD6 [[user(locn6)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> sceneColourTexture [[texture(0)]], sampler sceneColourSampler [[sampler(0)]])
{
    main0_out out = {};
    float4 _486;
    switch (0u)
    {
        default:
        {
            float2 _75 = _64;
            _75.x = in.in_var_TEXCOORD0.x;
            float2 _77 = _75;
            _77.y = in.in_var_TEXCOORD0.y;
            float4 _79 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0));
            float4 _81 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(0, 1));
            float _82 = _81.y;
            float4 _84 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(1, 0));
            float _85 = _84.y;
            float4 _87 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(0, -1));
            float _88 = _87.y;
            float4 _90 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(-1, 0));
            float _91 = _90.y;
            float _92 = _79.y;
            float _99 = fast::max(fast::max(_88, _91), fast::max(_85, fast::max(_82, _92)));
            float _102 = _99 - fast::min(fast::min(_88, _91), fast::min(_85, fast::min(_82, _92)));
            if (_102 < fast::max(0.0, _99 * 0.125))
            {
                _486 = _79;
                break;
            }
            float4 _108 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(-1));
            float _109 = _108.y;
            float4 _111 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(1));
            float _112 = _111.y;
            float4 _114 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(1, -1));
            float _115 = _114.y;
            float4 _117 = sceneColourTexture.sample(sceneColourSampler, _77, level(0.0), int2(-1, 1));
            float _118 = _117.y;
            float _119 = _88 + _82;
            float _120 = _91 + _85;
            float _123 = (-2.0) * _92;
            float _126 = _115 + _112;
            float _132 = _109 + _118;
            bool _152 = (abs(((-2.0) * _91) + _132) + ((abs(_123 + _119) * 2.0) + abs(((-2.0) * _85) + _126))) >= (abs(((-2.0) * _82) + (_118 + _112)) + ((abs(_123 + _120) * 2.0) + abs(((-2.0) * _88) + (_109 + _115))));
            bool _155 = !_152;
            float _156 = _155 ? _91 : _88;
            float _157 = _155 ? _85 : _82;
            float _161;
            if (_152)
            {
                _161 = in.in_var_TEXCOORD5.y;
            }
            else
            {
                _161 = in.in_var_TEXCOORD5.x;
            }
            float _168 = abs(_156 - _92);
            float _169 = abs(_157 - _92);
            bool _170 = _168 >= _169;
            float _175;
            if (_170)
            {
                _175 = -_161;
            }
            else
            {
                _175 = _161;
            }
            float _178 = fast::clamp(abs(((((_119 + _120) * 2.0) + (_132 + _126)) * 0.083333335816860198974609375) - _92) * (1.0 / _102), 0.0, 1.0);
            float _179 = _155 ? 0.0 : in.in_var_TEXCOORD5.x;
            float _181 = _152 ? 0.0 : in.in_var_TEXCOORD5.y;
            float2 _187;
            if (_155)
            {
                float2 _186 = _77;
                _186.x = in.in_var_TEXCOORD0.x + (_175 * 0.5);
                _187 = _186;
            }
            else
            {
                _187 = _77;
            }
            float2 _194;
            if (_152)
            {
                float2 _193 = _187;
                _193.y = _187.y + (_175 * 0.5);
                _194 = _193;
            }
            else
            {
                _194 = _187;
            }
            float _196 = _194.x - _179;
            float2 _197 = _64;
            _197.x = _196;
            float2 _200 = _197;
            _200.y = _194.y - _181;
            float _201 = _194.x + _179;
            float2 _202 = _64;
            _202.x = _201;
            float2 _204 = _202;
            _204.y = _194.y + _181;
            float _216 = fast::max(_168, _169) * 0.25;
            float _217 = ((!_170) ? (_157 + _92) : (_156 + _92)) * 0.5;
            float _219 = (((-2.0) * _178) + 3.0) * (_178 * _178);
            bool _220 = (_92 - _217) < 0.0;
            float _221 = sceneColourTexture.sample(sceneColourSampler, _200, level(0.0)).y - _217;
            float _222 = sceneColourTexture.sample(sceneColourSampler, _204, level(0.0)).y - _217;
            bool _227 = !(abs(_221) >= _216);
            float2 _233;
            if (_227)
            {
                float2 _232 = _200;
                _232.x = _196 - (_179 * 1.5);
                _233 = _232;
            }
            else
            {
                _233 = _200;
            }
            float2 _240;
            if (_227)
            {
                float2 _239 = _233;
                _239.y = _233.y - (_181 * 1.5);
                _240 = _239;
            }
            else
            {
                _240 = _233;
            }
            bool _241 = !(abs(_222) >= _216);
            float2 _248;
            if (_241)
            {
                float2 _247 = _204;
                _247.x = _201 + (_179 * 1.5);
                _248 = _247;
            }
            else
            {
                _248 = _204;
            }
            float2 _255;
            if (_241)
            {
                float2 _254 = _248;
                _254.y = _248.y + (_181 * 1.5);
                _255 = _254;
            }
            else
            {
                _255 = _248;
            }
            float2 _434;
            float2 _435;
            float _436;
            float _437;
            if (_227 || _241)
            {
                float _263;
                if (_227)
                {
                    _263 = sceneColourTexture.sample(sceneColourSampler, _240, level(0.0)).y;
                }
                else
                {
                    _263 = _221;
                }
                float _269;
                if (_241)
                {
                    _269 = sceneColourTexture.sample(sceneColourSampler, _255, level(0.0)).y;
                }
                else
                {
                    _269 = _222;
                }
                float _273;
                if (_227)
                {
                    _273 = _263 - _217;
                }
                else
                {
                    _273 = _263;
                }
                float _277;
                if (_241)
                {
                    _277 = _269 - _217;
                }
                else
                {
                    _277 = _269;
                }
                bool _282 = !(abs(_273) >= _216);
                float2 _289;
                if (_282)
                {
                    float2 _288 = _240;
                    _288.x = _240.x - (_179 * 2.0);
                    _289 = _288;
                }
                else
                {
                    _289 = _240;
                }
                float2 _296;
                if (_282)
                {
                    float2 _295 = _289;
                    _295.y = _289.y - (_181 * 2.0);
                    _296 = _295;
                }
                else
                {
                    _296 = _289;
                }
                bool _297 = !(abs(_277) >= _216);
                float2 _305;
                if (_297)
                {
                    float2 _304 = _255;
                    _304.x = _255.x + (_179 * 2.0);
                    _305 = _304;
                }
                else
                {
                    _305 = _255;
                }
                float2 _312;
                if (_297)
                {
                    float2 _311 = _305;
                    _311.y = _305.y + (_181 * 2.0);
                    _312 = _311;
                }
                else
                {
                    _312 = _305;
                }
                float2 _430;
                float2 _431;
                float _432;
                float _433;
                if (_282 || _297)
                {
                    float _320;
                    if (_282)
                    {
                        _320 = sceneColourTexture.sample(sceneColourSampler, _296, level(0.0)).y;
                    }
                    else
                    {
                        _320 = _273;
                    }
                    float _326;
                    if (_297)
                    {
                        _326 = sceneColourTexture.sample(sceneColourSampler, _312, level(0.0)).y;
                    }
                    else
                    {
                        _326 = _277;
                    }
                    float _330;
                    if (_282)
                    {
                        _330 = _320 - _217;
                    }
                    else
                    {
                        _330 = _320;
                    }
                    float _334;
                    if (_297)
                    {
                        _334 = _326 - _217;
                    }
                    else
                    {
                        _334 = _326;
                    }
                    bool _339 = !(abs(_330) >= _216);
                    float2 _346;
                    if (_339)
                    {
                        float2 _345 = _296;
                        _345.x = _296.x - (_179 * 4.0);
                        _346 = _345;
                    }
                    else
                    {
                        _346 = _296;
                    }
                    float2 _353;
                    if (_339)
                    {
                        float2 _352 = _346;
                        _352.y = _346.y - (_181 * 4.0);
                        _353 = _352;
                    }
                    else
                    {
                        _353 = _346;
                    }
                    bool _354 = !(abs(_334) >= _216);
                    float2 _362;
                    if (_354)
                    {
                        float2 _361 = _312;
                        _361.x = _312.x + (_179 * 4.0);
                        _362 = _361;
                    }
                    else
                    {
                        _362 = _312;
                    }
                    float2 _369;
                    if (_354)
                    {
                        float2 _368 = _362;
                        _368.y = _362.y + (_181 * 4.0);
                        _369 = _368;
                    }
                    else
                    {
                        _369 = _362;
                    }
                    float2 _426;
                    float2 _427;
                    float _428;
                    float _429;
                    if (_339 || _354)
                    {
                        float _377;
                        if (_339)
                        {
                            _377 = sceneColourTexture.sample(sceneColourSampler, _353, level(0.0)).y;
                        }
                        else
                        {
                            _377 = _330;
                        }
                        float _383;
                        if (_354)
                        {
                            _383 = sceneColourTexture.sample(sceneColourSampler, _369, level(0.0)).y;
                        }
                        else
                        {
                            _383 = _334;
                        }
                        float _387;
                        if (_339)
                        {
                            _387 = _377 - _217;
                        }
                        else
                        {
                            _387 = _377;
                        }
                        float _391;
                        if (_354)
                        {
                            _391 = _383 - _217;
                        }
                        else
                        {
                            _391 = _383;
                        }
                        bool _396 = !(abs(_387) >= _216);
                        float2 _403;
                        if (_396)
                        {
                            float2 _402 = _353;
                            _402.x = _353.x - (_179 * 12.0);
                            _403 = _402;
                        }
                        else
                        {
                            _403 = _353;
                        }
                        float2 _410;
                        if (_396)
                        {
                            float2 _409 = _403;
                            _409.y = _403.y - (_181 * 12.0);
                            _410 = _409;
                        }
                        else
                        {
                            _410 = _403;
                        }
                        bool _411 = !(abs(_391) >= _216);
                        float2 _418;
                        if (_411)
                        {
                            float2 _417 = _369;
                            _417.x = _369.x + (_179 * 12.0);
                            _418 = _417;
                        }
                        else
                        {
                            _418 = _369;
                        }
                        float2 _425;
                        if (_411)
                        {
                            float2 _424 = _418;
                            _424.y = _418.y + (_181 * 12.0);
                            _425 = _424;
                        }
                        else
                        {
                            _425 = _418;
                        }
                        _426 = _425;
                        _427 = _410;
                        _428 = _391;
                        _429 = _387;
                    }
                    else
                    {
                        _426 = _369;
                        _427 = _353;
                        _428 = _334;
                        _429 = _330;
                    }
                    _430 = _426;
                    _431 = _427;
                    _432 = _428;
                    _433 = _429;
                }
                else
                {
                    _430 = _312;
                    _431 = _296;
                    _432 = _277;
                    _433 = _273;
                }
                _434 = _430;
                _435 = _431;
                _436 = _432;
                _437 = _433;
            }
            else
            {
                _434 = _255;
                _435 = _240;
                _436 = _222;
                _437 = _221;
            }
            float _446;
            if (_155)
            {
                _446 = in.in_var_TEXCOORD0.y - _435.y;
            }
            else
            {
                _446 = in.in_var_TEXCOORD0.x - _435.x;
            }
            float _451;
            if (_155)
            {
                _451 = _434.y - in.in_var_TEXCOORD0.y;
            }
            else
            {
                _451 = _434.x - in.in_var_TEXCOORD0.x;
            }
            float _466 = fast::max(((_446 < _451) ? ((_437 < 0.0) != _220) : ((_436 < 0.0) != _220)) ? ((fast::min(_446, _451) * ((-1.0) / (_451 + _446))) + 0.5) : 0.0, (_219 * _219) * 0.75);
            float2 _472;
            if (_155)
            {
                float2 _471 = _77;
                _471.x = in.in_var_TEXCOORD0.x + (_466 * _175);
                _472 = _471;
            }
            else
            {
                _472 = _77;
            }
            float2 _479;
            if (_152)
            {
                float2 _478 = _472;
                _478.y = _472.y + (_466 * _175);
                _479 = _478;
            }
            else
            {
                _479 = _472;
            }
            _486 = float4(sceneColourTexture.sample(sceneColourSampler, _479, level(0.0)).xyz, _63);
            break;
        }
    }
    out.out_var_SV_Target0 = float4(mix(float3(dot(_486.xyz, float3(0.2125000059604644775390625, 0.7153999805450439453125, 0.07209999859333038330078125))), _486.xyz, float3(in.in_var_TEXCOORD6)), 1.0);
    return out;
}

