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
    highp vec4 u_screenParams;
    layout(row_major) highp mat4 u_inverseViewProjection;
    layout(row_major) highp mat4 u_inverseProjection;
    highp vec4 u_eyeToEarthSurfaceEyeSpace;
    highp vec4 u_outlineColour;
    highp vec4 u_outlineParams;
    highp vec4 u_colourizeHeightColourMin;
    highp vec4 u_colourizeHeightColourMax;
    highp vec4 u_colourizeHeightParams;
    highp vec4 u_colourizeDepthColour;
    highp vec4 u_colourizeDepthParams;
    highp vec4 u_contourColour;
    highp vec4 u_contourParams;
} u_fragParams;

uniform highp sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler;
uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

in highp vec4 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
in highp vec2 varying_TEXCOORD3;
in highp vec2 varying_TEXCOORD4;
in highp vec2 varying_TEXCOORD5;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec4 _90 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD1);
    highp vec4 _94 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, varying_TEXCOORD1);
    highp vec4 _98 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD1);
    highp float _99 = _98.x;
    highp float _105 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    highp float _108 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    highp float _110 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    highp float _120 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    highp float _122 = ((_105 + (_108 / (pow(2.0, _99 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear;
    highp vec4 _129 = vec4(varying_TEXCOORD0.xy, _122, 1.0) * u_fragParams.u_inverseProjection;
    highp vec4 _132 = _129 / vec4(_129.w);
    highp vec4 _140 = vec4(varying_TEXCOORD0.xy + vec2(0.0, u_fragParams.u_screenParams.y), _122, 1.0) * u_fragParams.u_inverseProjection;
    highp vec3 _144 = _90.xyz;
    highp vec3 _145 = _132.xyz;
    highp float _152 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    highp vec3 _154 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_145, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _152);
    highp float _162 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _154) * ((float(dot(_154, _154) < _152) * 2.0) - 1.0);
    highp float _205 = length((_140 / vec4(_140.w)) - _132);
    highp vec3 _269;
    switch (0u)
    {
        case 0u:
        {
            if (_205 == 0.0)
            {
                _269 = vec3(1.0, 1.0, 0.0);
                break;
            }
            highp vec3 _222;
            _222 = vec3(0.0);
            for (int _225 = 0; _225 < 16; )
            {
                _222 += (clamp(abs((fract(vec3(((abs(_162) + (_205 * (-4.0))) + ((_205 * 0.533333361148834228515625) * float(_225))) * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0);
                _225++;
                continue;
            }
            highp float _247 = _205 * 16.0;
            _269 = mix(mix(mix(mix(mix(_144, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_144, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_162 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_145) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, _222 * vec3(0.0625), vec3(clamp(u_fragParams.u_contourParams.w * clamp(u_fragParams.u_contourParams.z / _247, 0.0, 1.0), 0.0, 1.0))), u_fragParams.u_contourColour.xyz, vec3(((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(abs(_162)), u_fragParams.u_contourParams.x))) * clamp(u_fragParams.u_contourParams.x / _247, 0.0, 1.0)) * u_fragParams.u_contourColour.w));
            break;
        }
    }
    highp float _419;
    highp vec4 _420;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        highp vec4 _291 = vec4((varying_TEXCOORD1.x * 2.0) - 1.0, (varying_TEXCOORD1.y * 2.0) - 1.0, _122, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _296 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD2);
        highp float _297 = _296.x;
        highp vec4 _299 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD3);
        highp float _300 = _299.x;
        highp vec4 _302 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD4);
        highp float _303 = _302.x;
        highp vec4 _305 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD5);
        highp float _306 = _305.x;
        highp vec4 _321 = vec4((varying_TEXCOORD2.x * 2.0) - 1.0, (varying_TEXCOORD2.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _297 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _336 = vec4((varying_TEXCOORD3.x * 2.0) - 1.0, (varying_TEXCOORD3.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _300 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _351 = vec4((varying_TEXCOORD4.x * 2.0) - 1.0, (varying_TEXCOORD4.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _303 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _366 = vec4((varying_TEXCOORD5.x * 2.0) - 1.0, (varying_TEXCOORD5.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _306 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec3 _379 = (_291 / vec4(_291.w)).xyz;
        highp vec4 _416 = mix(vec4(_269, _99), vec4(mix(_269.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_297, _300), _303), _306)), vec4(1.0 - (((step(length(_379 - (_321 / vec4(_321.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_379 - (_336 / vec4(_336.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_351 / vec4(_351.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_366 / vec4(_366.w)).xyz), u_fragParams.u_outlineParams.y))));
        _419 = _416.w;
        _420 = vec4(_416.x, _416.y, _416.z, _90.w);
    }
    else
    {
        _419 = _99;
        _420 = vec4(_269.x, _269.y, _269.z, _90.w);
    }
    out_var_SV_Target0 = vec4(_420.xyz, 1.0);
    out_var_SV_Target1 = _94;
    gl_FragDepth = _419;
}

