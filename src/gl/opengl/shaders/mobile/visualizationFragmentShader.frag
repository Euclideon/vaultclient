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
uniform highp sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;

in highp vec4 varying_TEXCOORD0;
in highp vec2 varying_TEXCOORD1;
in highp vec2 varying_TEXCOORD2;
in highp vec2 varying_TEXCOORD3;
in highp vec2 varying_TEXCOORD4;
in highp vec2 varying_TEXCOORD5;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _78 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD1);
    highp vec4 _82 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD1);
    highp float _83 = _82.x;
    highp float _89 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    highp float _92 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    highp float _94 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    highp float _104 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    highp float _106 = ((_89 + (_92 / (pow(2.0, _83 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear;
    highp vec4 _112 = vec4(varying_TEXCOORD0.xy, _106, 1.0) * u_fragParams.u_inverseProjection;
    highp vec3 _116 = _78.xyz;
    highp vec3 _117 = (_112 / vec4(_112.w)).xyz;
    highp float _124 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    highp vec3 _126 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_117, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _124);
    highp float _132 = mix(length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _126), 0.0, float(dot(_126, _126) > _124));
    highp vec3 _207 = mix(mix(mix(mix(mix(_116, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_116, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_132 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_117) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_132 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_132), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    highp float _357;
    highp vec4 _358;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        highp vec4 _229 = vec4((varying_TEXCOORD1.x * 2.0) - 1.0, (varying_TEXCOORD1.y * 2.0) - 1.0, _106, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _234 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD2);
        highp float _235 = _234.x;
        highp vec4 _237 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD3);
        highp float _238 = _237.x;
        highp vec4 _240 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD4);
        highp float _241 = _240.x;
        highp vec4 _243 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD5);
        highp float _244 = _243.x;
        highp vec4 _259 = vec4((varying_TEXCOORD2.x * 2.0) - 1.0, (varying_TEXCOORD2.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _235 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _274 = vec4((varying_TEXCOORD3.x * 2.0) - 1.0, (varying_TEXCOORD3.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _238 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _289 = vec4((varying_TEXCOORD4.x * 2.0) - 1.0, (varying_TEXCOORD4.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _241 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _304 = vec4((varying_TEXCOORD5.x * 2.0) - 1.0, (varying_TEXCOORD5.y * 2.0) - 1.0, ((_89 + (_92 / (pow(2.0, _244 * _94) - 1.0))) * _104) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec3 _317 = (_229 / vec4(_229.w)).xyz;
        highp vec4 _354 = mix(vec4(_207, _83), vec4(mix(_207.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_235, _238), _241), _244)), vec4(1.0 - (((step(length(_317 - (_259 / vec4(_259.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_317 - (_274 / vec4(_274.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_289 / vec4(_289.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_317 - (_304 / vec4(_304.w)).xyz), u_fragParams.u_outlineParams.y))));
        _357 = _354.w;
        _358 = vec4(_354.x, _354.y, _354.z, _78.w);
    }
    else
    {
        _357 = _83;
        _358 = vec4(_207.x, _207.y, _207.z, _78.w);
    }
    highp vec4 _363 = vec4(_358.xyz, 1.0);
    _363.w = _357;
    out_var_SV_Target = _363;
    gl_FragDepth = _357;
}

