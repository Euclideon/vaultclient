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
uniform sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

layout(location = 0) in vec4 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 2) in vec2 in_var_TEXCOORD2;
layout(location = 3) in vec2 in_var_TEXCOORD3;
layout(location = 4) in vec2 in_var_TEXCOORD4;
layout(location = 5) in vec2 in_var_TEXCOORD5;
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec4 _78 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD1);
    vec4 _82 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD1);
    float _83 = _82.x;
    float _89 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    float _92 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    float _94 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    float _104 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    float _106 = ((_89 + (_92 / (pow(2.0, _83 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear;
    vec4 _112 = vec4(in_var_TEXCOORD0.xy, _106, 1.0) * u_fragParams.u_inverseProjection;
    vec3 _116 = _78.xyz;
    vec3 _117 = (_112 / vec4(_112.w)).xyz;
    float _124 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    vec3 _126 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_117, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _124);
    float _132 = mix(length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _126), 0.0, float(dot(_126, _126) > _124));
    vec3 _207 = mix(mix(mix(mix(mix(_116, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_116, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_132 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_117) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_132 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_132), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _357;
    vec4 _358;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        vec4 _229 = vec4((in_var_TEXCOORD1.x * 2.0) - 1.0, (in_var_TEXCOORD1.y * 2.0) - 1.0, _106, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _234 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD2);
        float _235 = _234.x;
        vec4 _237 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD3);
        float _238 = _237.x;
        vec4 _240 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD4);
        float _241 = _240.x;
        vec4 _243 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD5);
        float _244 = _243.x;
        vec4 _259 = vec4((in_var_TEXCOORD2.x * 2.0) - 1.0, (in_var_TEXCOORD2.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _235 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _274 = vec4((in_var_TEXCOORD3.x * 2.0) - 1.0, (in_var_TEXCOORD3.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _238 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _289 = vec4((in_var_TEXCOORD4.x * 2.0) - 1.0, (in_var_TEXCOORD4.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _241 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec4 _304 = vec4((in_var_TEXCOORD5.x * 2.0) - 1.0, (in_var_TEXCOORD5.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _244 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        vec3 _317 = (_229 / vec4(_229.w)).xyz;
        vec4 _354 = mix(vec4(_207, _83), vec4(mix(_207.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_235, _238), _241), _244)), vec4(1.0 - (((step(length(_317 - (_259 / vec4(_259.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_317 - (_274 / vec4(_274.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_289 / vec4(_289.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_304 / vec4(_304.w)).xyz), u_fragParams.u_outlineParams.y))));
        _357 = _354.w;
        _358 = vec4(_354.x, _354.y, _354.z, _78.w);
    }
    else
    {
        _357 = _83;
        _358 = vec4(_207.x, _207.y, _207.z, _78.w);
    }
    vec4 _363 = vec4(_358.xyz, 1.0);
    _363.w = _357;
    out_var_SV_Target = _363;
    gl_FragDepth = _357;
}

