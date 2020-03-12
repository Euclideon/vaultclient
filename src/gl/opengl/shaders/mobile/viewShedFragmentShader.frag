#version 300 es
precision highp float;
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

in vec2 v_uv;

out vec4 out_Colour;

uniform sampler2D u_depth;
uniform sampler2D u_shadowMapAtlas;

// Should match CPU
#define MAP_COUNT 3

layout (std140) uniform u_params
{
  mat4 u_shadowMapVP[MAP_COUNT];
  mat4 u_inverseProjection;
  vec4 u_visibleColour;
  vec4 u_notVisibleColour;
  vec4 u_viewDistance; // .yzw unused
};

float linearizeDepth(float depth)
{
  return (2.0 * s_CameraNearPlane) / (s_CameraFarPlane + s_CameraNearPlane - depth * (s_CameraFarPlane - s_CameraNearPlane));
}

float logToLinearDepth(float logDepth)
{
  float a = s_CameraFarPlane / (s_CameraFarPlane - s_CameraNearPlane);
  float b = s_CameraFarPlane * s_CameraNearPlane / (s_CameraNearPlane - s_CameraFarPlane);
  float worldDepth = pow(2.0, logDepth * log2(s_CameraFarPlane + 1.0)) - 1.0;
  return a + b / worldDepth;
}

void main()
{
  vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
  float logDepth = texture(u_depth, v_uv).x;
  float depth = logToLinearDepth(logDepth);

  vec4 fragEyePosition = u_inverseProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  fragEyePosition /= fragEyePosition.w;

  vec4 shadowUV = vec4(0.0);

  // unrolled loop
  vec4 shadowMapCoord0 = u_shadowMapVP[0] * vec4(fragEyePosition.xyz, 1.0);
  vec4 shadowMapCoord1 = u_shadowMapVP[1] * vec4(fragEyePosition.xyz, 1.0);
  vec4 shadowMapCoord2 = u_shadowMapVP[2] * vec4(fragEyePosition.xyz, 1.0);

  // note: z has no scale & biased because we are using a [0,1] depth projection matrix here
  vec3 shadowMapClip0 = (shadowMapCoord0.xyz / shadowMapCoord0.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
  vec3 shadowMapClip1 = (shadowMapCoord1.xyz / shadowMapCoord1.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
  vec3 shadowMapClip2 = (shadowMapCoord2.xyz / shadowMapCoord2.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);

  float isInMap0 = float(shadowMapClip0.x >= 0.0 && shadowMapClip0.x <= 1.0 && shadowMapClip0.y >= 0.0 && shadowMapClip0.y <= 1.0 && shadowMapClip0.z >= 0.0 && shadowMapClip0.z <= 1.0);
  float isInMap1 = float(shadowMapClip1.x >= 0.0 && shadowMapClip1.x <= 1.0 && shadowMapClip1.y >= 0.0 && shadowMapClip1.y <= 1.0 && shadowMapClip1.z >= 0.0 && shadowMapClip1.z <= 1.0);
  float isInMap2 = float(shadowMapClip2.x >= 0.0 && shadowMapClip2.x <= 1.0 && shadowMapClip2.y >= 0.0 && shadowMapClip2.y <= 1.0 && shadowMapClip2.z >= 0.0 && shadowMapClip2.z <= 1.0);

  // atlas UVs
  vec4 shadowMapUV0 = vec4((0.0 / float(MAP_COUNT)) + shadowMapClip0.x / float(MAP_COUNT), shadowMapClip0.y, shadowMapClip0.z, shadowMapCoord0.w);
  vec4 shadowMapUV1 = vec4((1.0 / float(MAP_COUNT)) + shadowMapClip1.x / float(MAP_COUNT), shadowMapClip1.y, shadowMapClip1.z, shadowMapCoord1.w);
  vec4 shadowMapUV2 = vec4((2.0 / float(MAP_COUNT)) + shadowMapClip2.x / float(MAP_COUNT), shadowMapClip2.y, shadowMapClip2.z, shadowMapCoord2.w);

  shadowUV = mix(shadowUV, shadowMapUV0, isInMap0);
  shadowUV = mix(shadowUV, shadowMapUV1, isInMap1);
  shadowUV = mix(shadowUV, shadowMapUV2, isInMap2);

  if (length(shadowUV.xyz) > 0.0 && (linearizeDepth(shadowUV.z) * s_CameraFarPlane) <= u_viewDistance.x)
  {
    // fragment is inside the view shed bounds
    float shadowMapLogDepth = texture(u_shadowMapAtlas, shadowUV.xy).x; // log z
  
    float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
    float logDepthSample = log2(1.0 + shadowUV.w) * halfFcoef;

    float diff = (0.00004 * s_CameraFarPlane) * (logDepthSample - shadowMapLogDepth);
    col = mix(u_visibleColour, u_notVisibleColour, clamp(diff, 0.0, 1.0));
  }

  out_Colour = vec4(col.xyz * col.w, 1.0); //additive
}
