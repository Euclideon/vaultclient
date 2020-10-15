#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

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
layout(location = 0) out vec4 out_var_COLOR0;
layout(location = 1) out vec2 out_var_TEXCOORD0;

vec4 _40;

void main()
{
    vec4 _162;
    vec4 _163;
    vec2 _164;
    switch (0u)
    {
        default:
        {
            float _54 = dot(in_var_POSITION.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
            float _56 = dot(in_var_COLOR0.xyz, u_EveryObject.u_nearPlane.xyz) - u_EveryObject.u_nearPlane.w;
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
            vec3 _93;
            vec3 _94;
            if (_72 == 1)
            {
                _93 = in_var_COLOR0.xyz;
                _94 = in_var_POSITION.xyz + ((in_var_COLOR0.xyz - in_var_POSITION.xyz) * ((-_54) / (_56 - _54)));
            }
            else
            {
                vec3 _92;
                if (_72 == 2)
                {
                    _92 = in_var_COLOR0.xyz + ((in_var_POSITION.xyz - in_var_COLOR0.xyz) * ((-_56) / (_54 - _56)));
                }
                else
                {
                    _92 = in_var_COLOR0.xyz;
                }
                _93 = _92;
                _94 = in_var_POSITION.xyz;
            }
            if (_72 == 3)
            {
                _162 = vec4(0.0);
                _163 = _40;
                _164 = vec2(0.0);
                break;
            }
            vec4 _108 = vec4(_94, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
            vec4 _109 = vec4(_93, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
            float _111 = _108.w;
            vec2 _113 = _108.xy / vec2(_111);
            vec2 _127;
            switch (0u)
            {
                default:
                {
                    vec2 _120 = _113 - (_109.xy / vec2(_109.w));
                    float _121 = length(_120);
                    if (_121 == 0.0)
                    {
                        _127 = vec2(1.0, 0.0);
                        break;
                    }
                    _127 = _120 / vec2(_121);
                    break;
                }
            }
            vec2 _133 = vec2(-_127.y, _127.x) * in_var_POSITION.w;
            vec2 _138 = _133;
            _138.x = _133.x / u_EveryObject.u_thickness.y;
            vec2 _142 = _113 + (_138 * u_EveryObject.u_thickness.x);
            float _147 = _108.z;
            vec2 _161;
            switch (0u)
            {
                default:
                {
                    if (u_EveryObject.u_worldViewProjectionMatrix[3u].y == 0.0)
                    {
                        _161 = vec2(_147, _111);
                        break;
                    }
                    _161 = vec2(1.0 + _111, 0.0);
                    break;
                }
            }
            _162 = vec4(_142.x * _111, _142.y * _111, _147, _111);
            _163 = u_EveryObject.u_colour;
            _164 = _161;
            break;
        }
    }
    gl_Position = _162;
    out_var_COLOR0 = _163;
    out_var_TEXCOORD0 = _164;
}

