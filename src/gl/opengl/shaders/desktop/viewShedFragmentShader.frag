#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(std140) uniform type_u_params
{
    layout(row_major) mat4 u_shadowMapVP[3];
    layout(row_major) mat4 u_inverseProjection;
    vec4 u_visibleColour;
    vec4 u_notVisibleColour;
    vec4 u_viewDistance;
} u_params;

uniform sampler2D SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler;
uniform sampler2D SPIRV_Cross_CombinedshadowMapAtlasTextureshadowMapAtlasSampler;

layout(location = 0) in vec4 in_var_TEXCOORD0;
layout(location = 1) in vec2 in_var_TEXCOORD1;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _62 = texture(SPIRV_Cross_CombinedsceneDepthTexturesceneDepthSampler, in_var_TEXCOORD1);
    float _68 = u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane;
    float _74 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    vec4 _92 = vec4(in_var_TEXCOORD0.xy, (((u_cameraPlaneParams.s_CameraFarPlane / _68) + (((u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane)) / (pow(2.0, _62.x * _74) - 1.0))) * (u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear)) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_params.u_inverseProjection;
    vec4 _101 = vec4((_92 / vec4(_92.w)).xyz, 1.0);
    vec4 _102 = _101 * u_params.u_shadowMapVP[0];
    vec4 _105 = _101 * u_params.u_shadowMapVP[1];
    vec4 _108 = _101 * u_params.u_shadowMapVP[2];
    float _110 = _102.w;
    vec3 _114 = ((_102.xyz / vec3(_110)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    float _116 = _105.w;
    vec3 _120 = ((_105.xyz / vec3(_116)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    float _122 = _108.w;
    vec3 _126 = ((_108.xyz / vec3(_122)) * vec3(0.5, 0.5, 1.0)) + vec3(0.5, 0.5, 0.0);
    float _127 = _114.x;
    float _131 = _114.y;
    float _136 = _114.z;
    float _142 = _120.x;
    float _146 = _120.y;
    float _151 = _120.z;
    float _157 = _126.x;
    float _161 = _126.y;
    float _166 = _126.z;
    vec4 _188 = mix(mix(mix(vec4(0.0), vec4(_127 * 0.3333333432674407958984375, 1.0 - _131, _136, _110), vec4(float((((((_127 >= 0.0) && (_127 <= 1.0)) && (_131 >= 0.0)) && (_131 <= 1.0)) && (_136 >= 0.0)) && (_136 <= 1.0)))), vec4(0.3333333432674407958984375 + (_142 * 0.3333333432674407958984375), 1.0 - _146, _151, _116), vec4(float((((((_142 >= 0.0) && (_142 <= 1.0)) && (_146 >= 0.0)) && (_146 <= 1.0)) && (_151 >= 0.0)) && (_151 <= 1.0)))), vec4(0.666666686534881591796875 + (_157 * 0.3333333432674407958984375), 1.0 - _161, _166, _122), vec4(float((((((_157 >= 0.0) && (_157 <= 1.0)) && (_161 >= 0.0)) && (_161 <= 1.0)) && (_166 >= 0.0)) && (_166 <= 1.0))));
    vec4 _226;
    if ((length(_188.xyz) > 0.0) && ((((2.0 * u_cameraPlaneParams.s_CameraNearPlane) / ((u_cameraPlaneParams.s_CameraFarPlane + u_cameraPlaneParams.s_CameraNearPlane) - (_188.z * _68))) * u_cameraPlaneParams.s_CameraFarPlane) <= u_params.u_viewDistance.x))
    {
        _226 = mix(u_params.u_visibleColour, u_params.u_notVisibleColour, vec4(clamp((3.9999998989515006542205810546875e-05 * u_cameraPlaneParams.s_CameraFarPlane) * ((log2(1.0 + _188.w) * (1.0 / _74)) - texture(SPIRV_Cross_CombinedshadowMapAtlasTextureshadowMapAtlasSampler, _188.xy).x), 0.0, 1.0)));
    }
    else
    {
        _226 = vec4(0.0);
    }
    out_var_SV_Target0 = vec4(_226.xyz * _226.w, 1.0);
    out_var_SV_Target1 = vec4(0.0);
}

