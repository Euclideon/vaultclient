#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

constant float _66 = {};
constant float2 _67 = {};
constant float2 _68 = {};

struct PEFOutput
{
  float4 out_var_SV_Target [[color(0)]];
};

struct PEFInput
{
  float2 in_var_TEXCOORD0 [[user(locn0)]];
  float2 in_var_TEXCOORD1 [[user(locn1)]];
  float2 in_var_TEXCOORD2 [[user(locn2)]];
  float2 in_var_TEXCOORD3 [[user(locn3)]];
  float2 in_var_TEXCOORD4 [[user(locn4)]];
  float2 in_var_TEXCOORD5 [[user(locn5)]];
  float in_var_TEXCOORD6 [[user(locn6)]];
};

fragment PEFOutput main0(PEFInput in [[stage_in]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
  PEFOutput out = {};
  float4 _80 = texture1.sample(sampler1, in.in_var_TEXCOORD0);
  float _81 = _80.x;
  float4 _535;
  if ((1.0 - (((step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD1).x - _81), 0.0030000000260770320892333984375) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD2).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD3).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD4).x - _81), 0.0030000000260770320892333984375))) == 0.0)
  {
    _535 = texture0.sample(sampler0, in.in_var_TEXCOORD0);
  }
  else
  {
    float4 _534;
    switch (0u)
    {
      default:
      {
        float2 _123 = _67;
        _123.x = in.in_var_TEXCOORD0.x;
        float2 _125 = _123;
        _125.y = in.in_var_TEXCOORD0.y;
        float4 _127 = texture0.sample(sampler0, _125, level(0.0));
        float4 _129 = texture0.sample(sampler0, _125, level(0.0), int2(0, 1));
        float _130 = _129.y;
        float4 _132 = texture0.sample(sampler0, _125, level(0.0), int2(1, 0));
        float _133 = _132.y;
        float4 _135 = texture0.sample(sampler0, _125, level(0.0), int2(0, -1));
        float _136 = _135.y;
        float4 _138 = texture0.sample(sampler0, _125, level(0.0), int2(-1, 0));
        float _139 = _138.y;
        float _140 = _127.y;
        float _147 = fast::max(fast::max(_136, _139), fast::max(_133, fast::max(_130, _140)));
        float _150 = _147 - fast::min(fast::min(_136, _139), fast::min(_133, fast::min(_130, _140)));
        if (_150 < fast::max(0.0, _147 * 0.125))
        {
          _534 = _127;
          break;
        }
        float4 _156 = texture0.sample(sampler0, _125, level(0.0), int2(-1));
        float _157 = _156.y;
        float4 _159 = texture0.sample(sampler0, _125, level(0.0), int2(1));
        float _160 = _159.y;
        float4 _162 = texture0.sample(sampler0, _125, level(0.0), int2(1, -1));
        float _163 = _162.y;
        float4 _165 = texture0.sample(sampler0, _125, level(0.0), int2(-1, 1));
        float _166 = _165.y;
        float _167 = _136 + _130;
        float _168 = _139 + _133;
        float _171 = (-2.0) * _140;
        float _174 = _163 + _160;
        float _180 = _157 + _166;
        bool _200 = (abs(((-2.0) * _139) + _180) + ((abs(_171 + _167) * 2.0) + abs(((-2.0) * _133) + _174))) >= (abs(((-2.0) * _130) + (_166 + _160)) + ((abs(_171 + _168) * 2.0) + abs(((-2.0) * _136) + (_157 + _163))));
        bool _203 = !_200;
        float _204 = _203 ? _139 : _136;
        float _205 = _203 ? _133 : _130;
        float _209;
        if (_200)
        {
          _209 = in.in_var_TEXCOORD5.y;
        }
        else
        {
          _209 = in.in_var_TEXCOORD5.x;
        }
        float _216 = abs(_204 - _140);
        float _217 = abs(_205 - _140);
        bool _218 = _216 >= _217;
        float _223;
        if (_218)
        {
          _223 = -_209;
        }
        else
        {
          _223 = _209;
        }
        float _226 = fast::clamp(abs(((((_167 + _168) * 2.0) + (_180 + _174)) * 0.083333335816860198974609375) - _140) * (1.0 / _150), 0.0, 1.0);
        float _227 = _203 ? 0.0 : in.in_var_TEXCOORD5.x;
        float _229 = _200 ? 0.0 : in.in_var_TEXCOORD5.y;
        float2 _235;
        if (_203)
        {
          float2 _234 = _125;
          _234.x = in.in_var_TEXCOORD0.x + (_223 * 0.5);
          _235 = _234;
        }
        else
        {
          _235 = _125;
        }
        float2 _242;
        if (_200)
        {
          float2 _241 = _235;
          _241.y = _235.y + (_223 * 0.5);
          _242 = _241;
        }
        else
        {
          _242 = _235;
        }
        float _244 = _242.x - _227;
        float2 _245 = _67;
        _245.x = _244;
        float2 _248 = _245;
        _248.y = _242.y - _229;
        float _249 = _242.x + _227;
        float2 _250 = _67;
        _250.x = _249;
        float2 _252 = _250;
        _252.y = _242.y + _229;
        float _264 = fast::max(_216, _217) * 0.25;
        float _265 = ((!_218) ? (_205 + _140) : (_204 + _140)) * 0.5;
        float _267 = (((-2.0) * _226) + 3.0) * (_226 * _226);
        bool _268 = (_140 - _265) < 0.0;
        float _269 = texture0.sample(sampler0, _248, level(0.0)).y - _265;
        float _270 = texture0.sample(sampler0, _252, level(0.0)).y - _265;
        bool _275 = !(abs(_269) >= _264);
        float2 _281;
        if (_275)
        {
          float2 _280 = _248;
          _280.x = _244 - (_227 * 1.5);
          _281 = _280;
        }
        else
        {
          _281 = _248;
        }
        float2 _288;
        if (_275)
        {
          float2 _287 = _281;
          _287.y = _281.y - (_229 * 1.5);
          _288 = _287;
        }
        else
        {
          _288 = _281;
        }
        bool _289 = !(abs(_270) >= _264);
        float2 _296;
        if (_289)
        {
          float2 _295 = _252;
          _295.x = _249 + (_227 * 1.5);
          _296 = _295;
        }
        else
        {
          _296 = _252;
        }
        float2 _303;
        if (_289)
        {
          float2 _302 = _296;
          _302.y = _296.y + (_229 * 1.5);
          _303 = _302;
        }
        else
        {
          _303 = _296;
        }
        float2 _482;
        float2 _483;
        float _484;
        float _485;
        if (_275 || _289)
        {
          float _311;
          if (_275)
          {
            _311 = texture0.sample(sampler0, _288, level(0.0)).y;
          }
          else
          {
            _311 = _269;
          }
          float _317;
          if (_289)
          {
            _317 = texture0.sample(sampler0, _303, level(0.0)).y;
          }
          else
          {
            _317 = _270;
          }
          float _321;
          if (_275)
          {
            _321 = _311 - _265;
          }
          else
          {
            _321 = _311;
          }
          float _325;
          if (_289)
          {
            _325 = _317 - _265;
          }
          else
          {
            _325 = _317;
          }
          bool _330 = !(abs(_321) >= _264);
          float2 _337;
          if (_330)
          {
            float2 _336 = _288;
            _336.x = _288.x - (_227 * 2.0);
            _337 = _336;
          }
          else
          {
            _337 = _288;
          }
          float2 _344;
          if (_330)
          {
            float2 _343 = _337;
            _343.y = _337.y - (_229 * 2.0);
            _344 = _343;
          }
          else
          {
            _344 = _337;
          }
          bool _345 = !(abs(_325) >= _264);
          float2 _353;
          if (_345)
          {
            float2 _352 = _303;
            _352.x = _303.x + (_227 * 2.0);
            _353 = _352;
          }
          else
          {
            _353 = _303;
          }
          float2 _360;
          if (_345)
          {
            float2 _359 = _353;
            _359.y = _353.y + (_229 * 2.0);
            _360 = _359;
          }
          else
          {
            _360 = _353;
          }
          float2 _478;
          float2 _479;
          float _480;
          float _481;
          if (_330 || _345)
          {
            float _368;
            if (_330)
            {
              _368 = texture0.sample(sampler0, _344, level(0.0)).y;
            }
            else
            {
              _368 = _321;
            }
            float _374;
            if (_345)
            {
              _374 = texture0.sample(sampler0, _360, level(0.0)).y;
            }
            else
            {
              _374 = _325;
            }
            float _378;
            if (_330)
            {
              _378 = _368 - _265;
            }
            else
            {
              _378 = _368;
            }
            float _382;
            if (_345)
            {
              _382 = _374 - _265;
            }
            else
            {
              _382 = _374;
            }
            bool _387 = !(abs(_378) >= _264);
            float2 _394;
            if (_387)
            {
              float2 _393 = _344;
              _393.x = _344.x - (_227 * 4.0);
              _394 = _393;
            }
            else
            {
              _394 = _344;
            }
            float2 _401;
            if (_387)
            {
              float2 _400 = _394;
              _400.y = _394.y - (_229 * 4.0);
              _401 = _400;
            }
            else
            {
              _401 = _394;
            }
            bool _402 = !(abs(_382) >= _264);
            float2 _410;
            if (_402)
            {
              float2 _409 = _360;
              _409.x = _360.x + (_227 * 4.0);
              _410 = _409;
            }
            else
            {
              _410 = _360;
            }
            float2 _417;
            if (_402)
            {
              float2 _416 = _410;
              _416.y = _410.y + (_229 * 4.0);
              _417 = _416;
            }
            else
            {
              _417 = _410;
            }
            float2 _474;
            float2 _475;
            float _476;
            float _477;
            if (_387 || _402)
            {
              float _425;
              if (_387)
              {
                _425 = texture0.sample(sampler0, _401, level(0.0)).y;
              }
              else
              {
                _425 = _378;
              }
              float _431;
              if (_402)
              {
                _431 = texture0.sample(sampler0, _417, level(0.0)).y;
              }
              else
              {
                _431 = _382;
              }
              float _435;
              if (_387)
              {
                _435 = _425 - _265;
              }
              else
              {
                _435 = _425;
              }
              float _439;
              if (_402)
              {
                _439 = _431 - _265;
              }
              else
              {
                _439 = _431;
              }
              bool _444 = !(abs(_435) >= _264);
              float2 _451;
              if (_444)
              {
                float2 _450 = _401;
                _450.x = _401.x - (_227 * 12.0);
                _451 = _450;
              }
              else
              {
                _451 = _401;
              }
              float2 _458;
              if (_444)
              {
                float2 _457 = _451;
                _457.y = _451.y - (_229 * 12.0);
                _458 = _457;
              }
              else
              {
                _458 = _451;
              }
              bool _459 = !(abs(_439) >= _264);
              float2 _466;
              if (_459)
              {
                float2 _465 = _417;
                _465.x = _417.x + (_227 * 12.0);
                _466 = _465;
              }
              else
              {
                _466 = _417;
              }
              float2 _473;
              if (_459)
              {
                float2 _472 = _466;
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
        float _494;
        if (_203)
        {
          _494 = in.in_var_TEXCOORD0.y - _483.y;
        }
        else
        {
          _494 = in.in_var_TEXCOORD0.x - _483.x;
        }
        float _499;
        if (_203)
        {
          _499 = _482.y - in.in_var_TEXCOORD0.y;
        }
        else
        {
          _499 = _482.x - in.in_var_TEXCOORD0.x;
        }
        float _514 = fast::max(((_494 < _499) ? ((_485 < 0.0) != _268) : ((_484 < 0.0) != _268)) ? ((fast::min(_494, _499) * ((-1.0) / (_499 + _494))) + 0.5) : 0.0, (_267 * _267) * 0.75);
        float2 _520;
        if (_203)
        {
          float2 _519 = _125;
          _519.x = in.in_var_TEXCOORD0.x + (_514 * _223);
          _520 = _519;
        }
        else
        {
          _520 = _125;
        }
        float2 _527;
        if (_200)
        {
          float2 _526 = _520;
          _526.y = _520.y + (_514 * _223);
          _527 = _526;
        }
        else
        {
          _527 = _520;
        }
        _534 = float4(texture0.sample(sampler0, _527, level(0.0)).xyz, _66);
        break;
      }
    }
    _535 = _534;
  }
  out.out_var_SV_Target = float4(mix(float3(dot(_535.xyz, float3(0.2125000059604644775390625, 0.7153999805450439453125, 0.07209999859333038330078125))), _535.xyz, float3(in.in_var_TEXCOORD6)), 1.0);
  return out;
}
