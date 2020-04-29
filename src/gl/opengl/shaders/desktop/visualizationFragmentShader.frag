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
    vec4 _81 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD1);
    vec4 _85 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, in_var_TEXCOORD1);
    vec4 _89 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD1);
    float _90 = _89.x;
    float _96 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _99 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _101 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _111 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _113 = ((_96 + (_99 / (pow(2.0, _90 * _101) - 1.0))) * _111) + u_cameraPlaneParams.u_clipZNear;
    vec4 _119 = vec4(in_var_TEXCOORD0.xy, _113, 1.0) * u_fragParams.u_inverseProjection;
    vec3 _123 = _81.xyz;
    vec3 _124 = (_119 / vec4(_119.w)).xyz;
    float _131 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    vec3 _133 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_124, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _131);
    float _139 = mix(length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _133), 0.0, float(dot(_133, _133) > _131));
    vec3 _214 = mix(mix(mix(mix(mix(_123, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_123, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_139 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_124) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_139 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_139), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _364;
    vec4 _365;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        vec4 _236 = vec4((in_var_TEXCOORD1.x * 2.0) - 1.0, (in_var_TEXCOORD1.y * 2.0) - 1.0, _113, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _241 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD2);
        float _242 = _241.x;
        vec4 _244 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD3);
        float _245 = _244.x;
        vec4 _247 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD4);
        float _248 = _247.x;
        vec4 _250 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD5);
        float _251 = _250.x;
        vec4 _266 = vec4((in_var_TEXCOORD2.x * 2.0) - 1.0, (in_var_TEXCOORD2.y * 2.0) - 1.0, ((_96 + (_99 / (pow(2.0, _242 * _101) - 1.0))) * _111) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _281 = vec4((in_var_TEXCOORD3.x * 2.0) - 1.0, (in_var_TEXCOORD3.y * 2.0) - 1.0, ((_96 + (_99 / (pow(2.0, _245 * _101) - 1.0))) * _111) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _296 = vec4((in_var_TEXCOORD4.x * 2.0) - 1.0, (in_var_TEXCOORD4.y * 2.0) - 1.0, ((_96 + (_99 / (pow(2.0, _248 * _101) - 1.0))) * _111) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _311 = vec4((in_var_TEXCOORD5.x * 2.0) - 1.0, (in_var_TEXCOORD5.y * 2.0) - 1.0, ((_96 + (_99 / (pow(2.0, _251 * _101) - 1.0))) * _111) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec3 _324 = (_236 / vec4(_236.w)).xyz;
        vec4 _361 = mix(vec4(_214, _90), vec4(mix(_214.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_242, _245), _248), _251)), vec4(1.0 - (((step(length(_324 - (_266 / vec4(_266.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_324 - (_281 / vec4(_281.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_324 - (_296 / vec4(_296.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_324 - (_311 / vec4(_311.w)).xyz), u_fragParams.u_outlineParams.y))));
        _364 = _361.w;
        _365 = vec4(_361.x, _361.y, _361.z, _81.w);
    }
    else
    {
        _364 = _90;
        _365 = vec4(_214.x, _214.y, _214.z, _81.w);
    }
    out_var_SV_Target0 = vec4(_365.xyz, 1.0);
    out_var_SV_Target1 = _85;
    gl_FragDepth = _364;
}

