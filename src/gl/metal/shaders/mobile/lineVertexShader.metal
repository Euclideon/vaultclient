#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_u_EveryObject
{
    float4 u_colour;
    float4 u_thickness;
    float4 u_nearPlane;
    float4x4 u_worldViewProjectionMatrix;
};

constant float2 _37 = {};
constant float4 _38 = {};

struct main0_out
{
    float4 out_var_COLOR0 [[user(locn0)]];
    float2 out_var_TEXCOORD0 [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 in_var_POSITION [[attribute(0)]];
    float4 in_var_COLOR0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant type_u_EveryObject& u_EveryObject [[buffer(0)]])
{
    main0_out out = {};
    float4 _152;
    float4 _153;
    float2 _154;
    switch (0u)
    {
        default:
        {
            float _52 = dot(in.in_var_POSITION.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            float _54 = dot(in.in_var_COLOR0.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            int _70;
            if (_52 > 0.0)
            {
                _70 = (_54 >= 0.0) ? 0 : 2;
            }
            else
            {
                int _69;
                if (_52 < 0.0)
                {
                    _69 = (_54 <= 0.0) ? 3 : 1;
                }
                else
                {
                    _69 = (_54 > 0.0) ? 0 : 3;
                }
                _70 = _69;
            }
            float3 _91;
            float3 _92;
            if (_70 == 1)
            {
                _91 = in.in_var_COLOR0.xyz;
                _92 = in.in_var_POSITION.xyz + ((in.in_var_COLOR0.xyz - in.in_var_POSITION.xyz) * ((-_52) / (_54 - _52)));
            }
            else
            {
                float3 _90;
                if (_70 == 2)
                {
                    _90 = in.in_var_COLOR0.xyz + ((in.in_var_POSITION.xyz - in.in_var_COLOR0.xyz) * ((-_54) / (_52 - _54)));
                }
                else
                {
                    _90 = in.in_var_COLOR0.xyz;
                }
                _91 = _90;
                _92 = in.in_var_POSITION.xyz;
            }
            if (_70 == 3)
            {
                float2 _96 = _37;
                _96.x = 0.0;
                _152 = float4(0.0);
                _153 = _38;
                _154 = _96;
                break;
            }
            float4 _107 = u_EveryObject.u_worldViewProjectionMatrix * float4(_92, 1.0);
            float4 _108 = u_EveryObject.u_worldViewProjectionMatrix * float4(_91, 1.0);
            float _110 = _107.w;
            float2 _112 = _107.xy / float2(_110);
            float2 _126;
            switch (0u)
            {
                default:
                {
                    float2 _119 = _112 - (_108.xy / float2(_108.w));
                    float _120 = length(_119);
                    if (_120 == 0.0)
                    {
                        _126 = float2(1.0, 0.0);
                        break;
                    }
                    _126 = _119 / float2(_120);
                    break;
                }
            }
            float2 _132 = float2(-_126.y, _126.x) * in.in_var_POSITION.w;
            float2 _137 = _132;
            _137.x = _132.x / u_EveryObject.u_thickness.y;
            float2 _141 = _112 + (_137 * u_EveryObject.u_thickness.x);
            float2 _151 = _37;
            _151.x = 1.0 + _110;
            _152 = float4(_141.x * _110, _141.y * _110, _107.z, _110);
            _153 = u_EveryObject.u_colour;
            _154 = _151;
            break;
        }
    }
    out.gl_Position = _152;
    out.out_var_COLOR0 = _153;
    out.out_var_TEXCOORD0 = _154;
    return out;
}

