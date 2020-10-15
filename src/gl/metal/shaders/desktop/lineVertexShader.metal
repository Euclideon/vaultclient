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

constant float4 _40 = {};

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
    float4 _162;
    float4 _163;
    float2 _164;
    switch (0u)
    {
        default:
        {
            float _54 = dot(in.in_var_POSITION.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            float _56 = dot(in.in_var_COLOR0.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            int _72;
            if (_54 > 0.0)
            {
                _72 = (_56 >= 0.0) ? 0 : 2;
            }
            else
            {
                int _71;
                if (_54 < 0.0)
                {
                    _71 = (_56 <= 0.0) ? 3 : 1;
                }
                else
                {
                    _71 = (_56 > 0.0) ? 0 : 3;
                }
                _72 = _71;
            }
            float3 _93;
            float3 _94;
            if (_72 == 1)
            {
                _93 = in.in_var_COLOR0.xyz;
                _94 = in.in_var_POSITION.xyz + ((in.in_var_COLOR0.xyz - in.in_var_POSITION.xyz) * ((-_54) / (_56 - _54)));
            }
            else
            {
                float3 _92;
                if (_72 == 2)
                {
                    _92 = in.in_var_COLOR0.xyz + ((in.in_var_POSITION.xyz - in.in_var_COLOR0.xyz) * ((-_56) / (_54 - _56)));
                }
                else
                {
                    _92 = in.in_var_COLOR0.xyz;
                }
                _93 = _92;
                _94 = in.in_var_POSITION.xyz;
            }
            if (_72 == 3)
            {
                _162 = float4(0.0);
                _163 = _40;
                _164 = float2(0.0);
                break;
            }
            float4 _108 = u_EveryObject.u_worldViewProjectionMatrix * float4(_94, 1.0);
            float4 _109 = u_EveryObject.u_worldViewProjectionMatrix * float4(_93, 1.0);
            float _111 = _108.w;
            float2 _113 = _108.xy / float2(_111);
            float2 _127;
            switch (0u)
            {
                default:
                {
                    float2 _120 = _113 - (_109.xy / float2(_109.w));
                    float _121 = length(_120);
                    if (_121 == 0.0)
                    {
                        _127 = float2(1.0, 0.0);
                        break;
                    }
                    _127 = _120 / float2(_121);
                    break;
                }
            }
            float2 _133 = float2(-_127.y, _127.x) * in.in_var_POSITION.w;
            float2 _138 = _133;
            _138.x = _133.x / u_EveryObject.u_thickness.y;
            float2 _142 = _113 + (_138 * u_EveryObject.u_thickness.x);
            float _147 = _108.z;
            float2 _161;
            switch (0u)
            {
                default:
                {
                    if (u_EveryObject.u_worldViewProjectionMatrix[1u][3u] == 0.0)
                    {
                        _161 = float2(_147, _111);
                        break;
                    }
                    _161 = float2(1.0 + _111, 0.0);
                    break;
                }
            }
            _162 = float4(_142.x * _111, _142.y * _111, _147, _111);
            _163 = u_EveryObject.u_colour;
            _164 = _161;
            break;
        }
    }
    out.gl_Position = _162;
    out.out_var_COLOR0 = _163;
    out.out_var_TEXCOORD0 = _164;
    return out;
}

