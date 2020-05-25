#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_fragParams
{
    vec4 u_screenParams;
    layout(row_major) mat4 u_inverseViewProjection;
    layout(row_major) mat4 u_inverseProjection;
    vec4 u_eyeToEarthSurfaceEyeSpace;
    vec4 u_outlineColour;
    vec4 u_outlineParams;
    vec4 u_colourizeHeightColourMin;
    vec4 u_colourizeHeightColourMax;
    vec4 u_colourizeHeightParams;
    vec4 u_colourizeDepthColour;
    vec4 u_colourizeDepthParams;
    vec4 u_contourColour;
    vec4 u_contourParams;
} u_fragParams;

uniform sampler2D SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler;
uniform sampler2D SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler;
uniform sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

layout(location = 0) in vec4 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec2 in_var_TEXCOORD2;
layout(location = 3) in vec2 in_var_TEXCOORD3;
layout(location = 4) in vec2 in_var_TEXCOORD4;
layout(location = 5) in vec2 in_var_TEXCOORD5;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _90 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD1);
    vec4 _94 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, in_var_TEXCOORD1);
    vec4 _98 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD1);
    float _99 = _98.x;
    float _105 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _108 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _110 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _120 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _122 = ((_105 + (_108 / (pow(2.0, _99 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear;
    vec4 _129 = vec4(in_var_TEXCOORD0.xy, _122, 1.0) * u_fragParams.u_inverseProjection;
    vec4 _132 = _129 / vec4(_129.w);
    vec4 _140 = vec4(in_var_TEXCOORD0.xy + vec2(0.0, u_fragParams.u_screenParams.y), _122, 1.0) * u_fragParams.u_inverseProjection;
    vec3 _144 = _90.xyz;
    vec3 _145 = _132.xyz;
    float _152 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    vec3 _154 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_145, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _152);
    float _162 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _154) * ((float(dot(_154, _154) < _152) * 2.0) - 1.0);
    float _205 = length((_140 / vec4(_140.w)) - _132);
    vec3 _269;
    switch (0u)
    {
        default:
        {
            if (_205 == 0.0)
            {
                _269 = vec3(1.0, 1.0, 0.0);
                break;
            }
            vec3 _222;
            _222 = vec3(0.0);
            for (int _225 = 0; _225 < 16; )
            {
                _222 += (clamp(abs((fract(vec3(((abs(_162) + (_205 * (-4.0))) + ((_205 * 0.533333361148834228515625) * float(_225))) * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0);
                _225++;
                continue;
            }
            float _247 = _205 * 16.0;
            _269 = mix(mix(mix(mix(mix(_144, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_144, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_162 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_145) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, _222 * vec3(0.0625), vec3(clamp(u_fragParams.u_contourParams.w * clamp(u_fragParams.u_contourParams.z / _247, 0.0, 1.0), 0.0, 1.0))), u_fragParams.u_contourColour.xyz, vec3(((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(abs(_162)), u_fragParams.u_contourParams.x))) * clamp(u_fragParams.u_contourParams.x / _247, 0.0, 1.0)) * u_fragParams.u_contourColour.w));
            break;
        }
    }
    float _419;
    vec4 _420;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        vec4 _291 = vec4((in_var_TEXCOORD1.x * 2.0) - 1.0, (in_var_TEXCOORD1.y * 2.0) - 1.0, _122, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _296 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD2);
        float _297 = _296.x;
        vec4 _299 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD3);
        float _300 = _299.x;
        vec4 _302 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD4);
        float _303 = _302.x;
        vec4 _305 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD5);
        float _306 = _305.x;
        vec4 _321 = vec4((in_var_TEXCOORD2.x * 2.0) - 1.0, (in_var_TEXCOORD2.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _297 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _336 = vec4((in_var_TEXCOORD3.x * 2.0) - 1.0, (in_var_TEXCOORD3.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _300 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _351 = vec4((in_var_TEXCOORD4.x * 2.0) - 1.0, (in_var_TEXCOORD4.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _303 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _366 = vec4((in_var_TEXCOORD5.x * 2.0) - 1.0, (in_var_TEXCOORD5.y * 2.0) - 1.0, ((_105 + (_108 / (pow(2.0, _306 * _110) - 1.0))) * _120) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec3 _379 = (_291 / vec4(_291.w)).xyz;
        vec4 _416 = mix(vec4(_269, _99), vec4(mix(_269.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_297, _300), _303), _306)), vec4(1.0 - (((step(length(_379 - (_321 / vec4(_321.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_379 - (_336 / vec4(_336.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_351 / vec4(_351.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_379 - (_366 / vec4(_366.w)).xyz), u_fragParams.u_outlineParams.y))));
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

