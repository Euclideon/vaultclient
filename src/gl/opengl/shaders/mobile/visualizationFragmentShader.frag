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

uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;
uniform highp sampler2D SPIRV_Cross_Combinedtexture1sampler1;

in highp vec4 in_var_TEXCOORD0;
in highp vec2 in_var_TEXCOORD1;
in highp vec2 in_var_TEXCOORD2;
in highp vec2 in_var_TEXCOORD3;
in highp vec2 in_var_TEXCOORD4;
in highp vec2 in_var_TEXCOORD5;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _77 = texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD1);
    highp vec4 _81 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD1);
    highp float _82 = _81.x;
    highp float _88 = u_cameraPlaneParams.s_CameraFarPlane / (u_cameraPlaneParams.s_CameraFarPlane - u_cameraPlaneParams.s_CameraNearPlane);
    highp float _91 = (u_cameraPlaneParams.s_CameraFarPlane * u_cameraPlaneParams.s_CameraNearPlane) / (u_cameraPlaneParams.s_CameraNearPlane - u_cameraPlaneParams.s_CameraFarPlane);
    highp float _93 = log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0);
    highp float _103 = u_cameraPlaneParams.u_clipZFar - u_cameraPlaneParams.u_clipZNear;
    highp float _105 = ((_88 + (_91 / (pow(2.0, _82 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear;
    highp vec4 _110 = vec4(in_var_TEXCOORD0.xy, _105, 1.0);
    highp vec4 _111 = _110 * u_fragParams.u_inverseViewProjection;
    highp vec4 _114 = _111 / vec4(_111.w);
    highp vec4 _117 = _110 * u_fragParams.u_inverseProjection;
    highp vec3 _121 = _77.xyz;
    highp float _124 = _114.z;
    highp vec3 _200 = mix(mix(mix(mix(mix(_121, u_fragParams.u_colourizeHeightColourMin.xyz, vec3(u_fragParams.u_colourizeHeightColourMin.w)), mix(_121, u_fragParams.u_colourizeHeightColourMax.xyz, vec3(u_fragParams.u_colourizeHeightColourMax.w)), vec3(clamp((_124 - u_fragParams.u_colourizeHeightParams.x) / (u_fragParams.u_colourizeHeightParams.y - u_fragParams.u_colourizeHeightParams.x), 0.0, 1.0))).xyz, u_fragParams.u_colourizeDepthColour.xyz, vec3(clamp((length((_117 / vec4(_117.w)).xyz) - u_fragParams.u_colourizeDepthParams.x) / (u_fragParams.u_colourizeDepthParams.y - u_fragParams.u_colourizeDepthParams.x), 0.0, 1.0) * u_fragParams.u_colourizeDepthColour.w)).xyz, clamp(abs((fract(vec3(_124 * (1.0 / u_fragParams.u_contourParams.z), 1.0, 1.0).xxx + vec3(1.0, 0.666666686534881591796875, 0.3333333432674407958984375)) * 6.0) - vec3(3.0)) - vec3(1.0), vec3(0.0), vec3(1.0)) * 1.0, vec3(u_fragParams.u_contourParams.w)), u_fragParams.u_contourColour.xyz, vec3((1.0 - step(u_fragParams.u_contourParams.y, mod(abs(_124), u_fragParams.u_contourParams.x))) * u_fragParams.u_contourColour.w));
    highp float _350;
    highp vec4 _351;
    if ((u_fragParams.u_outlineParams.x > 0.0) && (u_fragParams.u_outlineColour.w > 0.0))
    {
        highp vec4 _222 = vec4((in_var_TEXCOORD1.x * 2.0) - 1.0, (in_var_TEXCOORD1.y * 2.0) - 1.0, _105, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _227 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD2);
        highp float _228 = _227.x;
        highp vec4 _230 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD3);
        highp float _231 = _230.x;
        highp vec4 _233 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD4);
        highp float _234 = _233.x;
        highp vec4 _236 = texture(SPIRV_Cross_Combinedtexture1sampler1, in_var_TEXCOORD5);
        highp float _237 = _236.x;
        highp vec4 _252 = vec4((in_var_TEXCOORD2.x * 2.0) - 1.0, (in_var_TEXCOORD2.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _228 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _267 = vec4((in_var_TEXCOORD3.x * 2.0) - 1.0, (in_var_TEXCOORD3.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _231 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _282 = vec4((in_var_TEXCOORD4.x * 2.0) - 1.0, (in_var_TEXCOORD4.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _234 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec4 _297 = vec4((in_var_TEXCOORD5.x * 2.0) - 1.0, (in_var_TEXCOORD5.y * 2.0) - 1.0, ((_88 + (_91 / (pow(2.0, _237 * _93) - 1.0))) * _103) + u_cameraPlaneParams.u_clipZNear, 1.0) * u_fragParams.u_inverseProjection;
        highp vec3 _310 = (_222 / vec4(_222.w)).xyz;
        highp vec4 _347 = mix(vec4(_200, _82), vec4(mix(_200.xyz, u_fragParams.u_outlineColour.xyz, vec3(u_fragParams.u_outlineColour.w)), min(min(min(_228, _231), _234), _237)), vec4(1.0 - (((step(length(_310 - (_252 / vec4(_252.w)).xyz), u_fragParams.u_outlineParams.y) * step(length(_310 - (_267 / vec4(_267.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_310 - (_282 / vec4(_282.w)).xyz), u_fragParams.u_outlineParams.y)) * step(length(_310 - (_297 / vec4(_297.w)).xyz), u_fragParams.u_outlineParams.y))));
        _350 = _347.w;
        _351 = vec4(_347.x, _347.y, _347.z, _77.w);
    }
    else
    {
        _350 = _82;
        _351 = vec4(_200.x, _200.y, _200.z, _77.w);
    }
    out_var_SV_Target = vec4(_351.xyz, 1.0);
    gl_FragDepth = _350;
}

