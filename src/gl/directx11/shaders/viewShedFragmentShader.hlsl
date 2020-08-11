cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 clip : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

sampler sceneDepthSampler;
Texture2D sceneDepthTexture;

sampler shadowMapAtlasSampler;
Texture2D shadowMapAtlasTexture;

// Should match CPU
#define MAP_COUNT 3

cbuffer u_params : register(b0)
{
  float4x4 u_shadowMapVP[MAP_COUNT];
  float4x4 u_inverseProjection;
  float4 u_visibleColour;
  float4 u_notVisibleColour;
  float4 u_viewDistance; // .yzw unused
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

float linearDepthToClipZ(float depth)
{
  return depth * (u_clipZFar - u_clipZNear) + u_clipZNear;
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 col = float4(0.0, 0.0, 0.0, 0.0);
  float logDepth = sceneDepthTexture.Sample(sceneDepthSampler, input.uv).x;
  float clipZ = linearDepthToClipZ(logToLinearDepth(logDepth));

  float4 fragEyePosition = mul(u_inverseProjection, float4(input.clip.xy, clipZ, 1.0));
  fragEyePosition /= fragEyePosition.w;

  float4 shadowUV = float4(0, 0, 0, 0);

  // unrolled loop
  float4 shadowMapCoord0 = mul(u_shadowMapVP[0], float4(fragEyePosition.xyz, 1.0));
  float4 shadowMapCoord1 = mul(u_shadowMapVP[1], float4(fragEyePosition.xyz, 1.0));
  float4 shadowMapCoord2 = mul(u_shadowMapVP[2], float4(fragEyePosition.xyz, 1.0));

  // note: z has no scale & biased because we are using a [0, 1] depth projection matrix here
  float3 shadowMapClip0 = (shadowMapCoord0.xyz / shadowMapCoord0.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);
  float3 shadowMapClip1 = (shadowMapCoord1.xyz / shadowMapCoord1.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);
  float3 shadowMapClip2 = (shadowMapCoord2.xyz / shadowMapCoord2.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);

  float isInMap0 = float(shadowMapClip0.x >= 0.0 && shadowMapClip0.x <= 1.0 && shadowMapClip0.y >= 0.0 && shadowMapClip0.y <= 1.0 && shadowMapClip0.z >= 0.0 && shadowMapClip0.z <= 1.0);
  float isInMap1 = float(shadowMapClip1.x >= 0.0 && shadowMapClip1.x <= 1.0 && shadowMapClip1.y >= 0.0 && shadowMapClip1.y <= 1.0 && shadowMapClip1.z >= 0.0 && shadowMapClip1.z <= 1.0);
  float isInMap2 = float(shadowMapClip2.x >= 0.0 && shadowMapClip2.x <= 1.0 && shadowMapClip2.y >= 0.0 && shadowMapClip2.y <= 1.0 && shadowMapClip2.z >= 0.0 && shadowMapClip2.z <= 1.0);

  // atlas UVs
  float4 shadowMapUV0 = float4((0.0 / float(MAP_COUNT)) + shadowMapClip0.x / float(MAP_COUNT), 1.0 - shadowMapClip0.y, shadowMapClip0.z, shadowMapCoord0.w);
  float4 shadowMapUV1 = float4((1.0 / float(MAP_COUNT)) + shadowMapClip1.x / float(MAP_COUNT), 1.0 - shadowMapClip1.y, shadowMapClip1.z, shadowMapCoord1.w);
  float4 shadowMapUV2 = float4((2.0 / float(MAP_COUNT)) + shadowMapClip2.x / float(MAP_COUNT), 1.0 - shadowMapClip2.y, shadowMapClip2.z, shadowMapCoord2.w);

  shadowUV = lerp(shadowUV, shadowMapUV0, isInMap0);
  shadowUV = lerp(shadowUV, shadowMapUV1, isInMap1);
  shadowUV = lerp(shadowUV, shadowMapUV2, isInMap2);

  if (length(shadowUV.xyz) > 0.0 && (linearizeDepth(shadowUV.z) * s_CameraFarPlane) <= u_viewDistance.x)
  {
    // fragment is inside the view shed bounds
    float shadowMapLogDepth = shadowMapAtlasTexture.Sample(shadowMapAtlasSampler, shadowUV.xy).x;// log z

    float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
    float logDepthSample = log2(1.0 + shadowUV.w) * halfFcoef;

    float diff = (0.00004 * s_CameraFarPlane) * (logDepthSample - shadowMapLogDepth);
    col = lerp(u_visibleColour, u_notVisibleColour, clamp(diff, 0.0, 1.0));
  }

  //additive
  output.Color0 = float4(col.xyz * col.w, 1.0);
  output.Normal = float4(0, 0, 0, 0);
  return output;
}
