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
    vec4 _83 = texture(SPIRV_Cross_CombinedsceneColourTexturesceneColourSampler, in_var_TEXCOORD1);
    vec4 _87 = texture(SPIRV_Cross_CombinedsceneNormalTexturesceneNormalSampler, in_var_TEXCOORD1);
    vec4 _91 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD1);
    float _92 = _91.x;
    vec4 _121 = vec4(in_var_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane)) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _92 * log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0)) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
    vec3 _125 = _83.xyz;
    vec3 _126 = (_121 / vec4(_121.w)).xyz;
    float _133 = dot(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz);
    vec3 _135 = u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz * (dot(_126, u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz) / _133);
    float _143 = length(u_fragParams.u_eyeToEarthSurfaceEyeSpace.xyz - _135) * ((float(dot(_135, _135) < _133) * 2.0) - 1.0);
    float _193 = abs(_143);
    vec3 _219 = mix(mix(mix(mix(mix(_125, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_125, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_143 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length(_126) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_193 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_193), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    float _284;
    vec4 _285;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        vec4 _235 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD2);
        float _236 = _235.x;
        vec4 _238 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD3);
        float _239 = _238.x;
        vec4 _241 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD4);
        float _242 = _241.x;
        vec4 _244 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD5);
        float _245 = _244.x;
        float _248 = 0.0005000000237487256526947021484375 + ((u_fragParams.u_outlineParams.y * _92) * 0.001000000047497451305389404296875);
        vec4 _281 = mix(vec4(_219, _92), vec4(mix(_219.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_236, _239), _242), _245)), vec4(1.0 - (((step(abs(_236 - _92), _248) * step(abs(_239 - _92), _248)) * step(abs(_242 - _92), _248)) * step(abs(_245 - _92), _248))));
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

