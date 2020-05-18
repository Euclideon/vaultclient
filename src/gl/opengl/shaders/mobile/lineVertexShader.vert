#version 300 es

layout(std140) uniform type_u_EveryObject
{
    vec4 u_colour;
    vec4 u_thickness;
    vec4 u_nearPlane;
    layout(row_major) mat4 u_worldViewProjectionMatrix;
} u_EveryObject;

layout(location = 0) in vec4 in_var_POSITION;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 2) in vec4 in_var_COLOR1;
out vec4 varying_COLOR0;
out vec2 varying_TEXCOORD0;

vec2 _37;
vec4 _38;

void main()
{
    vec4 _152;
    vec4 _153;
    vec2 _154;
    switch (0u)
    {
        case 0u:
        {
            float _52 = dot(in_var_POSITION.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            float _54 = dot(in_var_COLOR0.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
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
            vec3 _91;
            vec3 _92;
            if (_70 == 1)
            {
                _91 = in_var_COLOR0.xyz;
                _92 = in_var_POSITION.xyz + ((in_var_COLOR0.xyz - in_var_POSITION.xyz) * ((-_52) / (_54 - _52)));
            }
            else
            {
                vec3 _90;
                if (_70 == 2)
                {
                    _90 = in_var_COLOR0.xyz + ((in_var_POSITION.xyz - in_var_COLOR0.xyz) * ((-_54) / (_52 - _54)));
                }
                else
                {
                    _90 = in_var_COLOR0.xyz;
                }
                _91 = _90;
                _92 = in_var_POSITION.xyz;
            }
            if (_70 == 3)
            {
                vec2 _96 = _37;
                _96.x = 0.0;
                _152 = vec4(0.0);
                _153 = _38;
                _154 = _96;
                break;
            }
            vec4 _107 = vec4(_92, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
            vec4 _108 = vec4(_91, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
            float _110 = _107.w;
            vec2 _112 = _107.xy / vec2(_110);
            vec2 _126;
            switch (0u)
            {
                case 0u:
                {
                    vec2 _119 = _112 - (_108.xy / vec2(_108.w));
                    float _120 = length(_119);
                    if (_120 == 0.0)
                    {
                        _126 = vec2(1.0, 0.0);
                        break;
                    }
                    _126 = _119 / vec2(_120);
                    break;
                }
            }
            vec2 _132 = vec2(-_126.y, _126.x) * in_var_POSITION.w;
            vec2 _137 = _132;
            _137.x = _132.x / u_EveryObject.u_thickness.y;
            vec2 _141 = _112 + (_137 * u_EveryObject.u_thickness.x);
            vec2 _151 = _37;
            _151.x = 1.0 + _110;
            _152 = vec4(_141.x * _110, _141.y * _110, _107.z, _110);
            _153 = u_EveryObject.u_colour;
            _154 = _151;
            break;
        }
    }
    gl_Position = _152;
    varying_COLOR0 = _153;
    varying_TEXCOORD0 = _154;
}

