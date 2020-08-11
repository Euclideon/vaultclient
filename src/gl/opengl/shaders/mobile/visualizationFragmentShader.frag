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
    highp vec4 _83 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, varying_TEXCOORD1);
    highp vec4 _87 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, varying_TEXCOORD1);
    highp vec4 _91 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD1);
    highp float _92 = _91.x;
    highp vec4 _121 = vec4(varying_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane)) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _92 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
    highp vec3 _125 = _83.xyz;
    highp vec3 _126 = (_121 / vec4(_121.w)).xyz;
    highp float _133 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    highp vec3 _135 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_126, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _133);
    highp float _143 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _135) * ((float(dot(_135, _135) < _133) * 2.0) - 1.0);
    highp float _193 = abs(_143);
    highp vec3 _219 = mix(mix(mix(mix(mix(_125, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_125, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_143 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_126) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_193 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_193), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    highp float _284;
    highp vec4 _285;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        highp vec4 _235 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD2);
        highp float _236 = _235.x;
        highp vec4 _238 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD3);
        highp float _239 = _238.x;
        highp vec4 _241 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD4);
        highp float _242 = _241.x;
        highp vec4 _244 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, varying_TEXCOORD5);
        highp float _245 = _244.x;
        highp float _248 = 0.0005000000237487256526947021484375 + ((u_fragParams.u_outlineParams.y * _92) * 0.001000000047497451305389404296875);
        highp vec4 _281 = mix(vec4(_219, _92), vec4(mix(_219.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_236, _239), _242), _245)), vec4(1.0 - (((step(abs(_236 - _92), _248) * step(abs(_239 - _92), _248)) * step(abs(_242 - _92), _248)) * step(abs(_245 - _92), _248))));
        _284 = _281.w;
        _285 = vec4(_281.x, _281.y, _281.z, _83.w);
    }
    else
    {
        _284 = _92;
        _285 = vec4(_219.x, _219.y, _219.z, _83.w);
    }
    out_var_SV_Target0 = vec4(_285.xyz, 1.0);
    out_var_SV_Target1 = _87;
    gl_FragDepth = _284;
}

