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
  float2 edgeSampleUV0 : TEXCOORD2;
  float2 edgeSampleUV1 : TEXCOORD3;
  float2 edgeSampleUV2 : TEXCOORD4;
  float2 edgeSampleUV3 : TEXCOORD5;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

sampler sceneColourSampler;
Texture2D sceneColourTexture;

sampler sceneNormalSampler;
Texture2D sceneNormalTexture;

sampler sceneDepthSampler;
Texture2D sceneDepthTexture;

cbuffer u_fragParams : register(b0)
{
  float4 u_screenParams;  // sampleStepSizeX, sampleStepSizeY, (unused), (unused)
  float4x4 u_inverseViewProjection;
  float4x4 u_inverseProjection;
  float4 u_eyeToEarthSurfaceEyeSpace;
  
  // outlining
  float4 u_outlineColour;
  float4 u_outlineParams;   // outlineWidth, threshold, (unused), (unused)

  // colour by height
  float4 u_colourizeHeightColourMin;
  float4 u_colourizeHeightColourMax;
  float4 u_colourizeHeightParams; // min world height, max world height, (unused), (unused)

  // colour by depth
  float4 u_colourizeDepthColour;
  float4 u_colourizeDepthParams; // min distance, max distance, (unused), (unused)

  // contours
  float4 u_contourColour;
  float4 u_contourParams; // contour distance, contour band height, contour rainbow repeat rate, contour rainbow factoring
};

float logToLinearDepth(float logDepth)
{
  float a = s_CameraFarPlane / (s_CameraFarPlane - s_CameraNearPlane);
  float b = s_CameraFarPlane * s_CameraNearPlane / (s_CameraNearPlane - s_CameraFarPlane);
  float worldDepth = pow(2.0, logDepth * log2(s_CameraFarPlane + 1.0)) - 1.0;
  return (a + b / worldDepth);
}

float linearDepthToClipZ(float depth)
{
  return depth * (u_clipZFar - u_clipZNear) + u_clipZNear;
}

float3 unpackNormal(float4 normalPacked)
{
  return float3(normalPacked.x, normalPacked.y,
                sign(normalPacked.w) * sqrt(1 - dot(normalPacked.xy, normalPacked.xy)));
}

float getNormalizedPosition(float v, float min, float max)
{
  return clamp((v - min) / (max - min), 0.0, 1.0);
}

// note: an adjusted depth is packed into the returned .w component
// this is to show the edge highlights against the skybox
float4 edgeHighlight(PS_INPUT input, float3 col, float depth, float logDepth, float4 outlineColour, float edgeOutlineThreshold)
{
  float3 n = unpackNormal(sceneNormalTexture.Sample(sceneNormalSampler, input.uv));
  float3 n0 = unpackNormal(sceneNormalTexture.Sample(sceneNormalSampler, input.edgeSampleUV0));
  float3 n1 = unpackNormal(sceneNormalTexture.Sample(sceneNormalSampler, input.edgeSampleUV1));
  float3 n2 = unpackNormal(sceneNormalTexture.Sample(sceneNormalSampler, input.edgeSampleUV2));
  float3 n3 = unpackNormal(sceneNormalTexture.Sample(sceneNormalSampler, input.edgeSampleUV3));
  
  float isEdge = 1.0 - step(edgeOutlineThreshold, dot(n, n0)) * step(edgeOutlineThreshold, dot(n, n1)) *step(edgeOutlineThreshold, dot(n, n2)) *step(edgeOutlineThreshold, dot(n, n3));
  
  //return float4(float() <= edgeThreshold), 0, 0, 1.0);
  
  //float4 eyePosition = mul(u_inverseProjection, float4(input.uv.x * 2.0 - 1.0, input.uv.y * 2.0 - 1.0, linearDepthToClipZ(depth), 1.0));
  //eyePosition /= eyePosition.w;

  //float sampleDepth0 = sceneDepthTexture.Sample(sceneDepthSampler, input.edgeSampleUV0).x;
  //float sampleDepth1 = sceneDepthTexture.Sample(sceneDepthSampler, input.edgeSampleUV1).x;
  //float sampleDepth2 = sceneDepthTexture.Sample(sceneDepthSampler, input.edgeSampleUV2).x;
  //float sampleDepth3 = sceneDepthTexture.Sample(sceneDepthSampler, input.edgeSampleUV3).x;
  //
  //float4 eyePosition0 = mul(u_inverseProjection, float4(input.edgeSampleUV0.x * 2.0 - 1.0, input.edgeSampleUV0.y * 2.0 - 1.0, linearDepthToClipZ(logToLinearDepth(sampleDepth0)), 1.0));
  //float4 eyePosition1 = mul(u_inverseProjection, float4(input.edgeSampleUV1.x * 2.0 - 1.0, input.edgeSampleUV1.y * 2.0 - 1.0, linearDepthToClipZ(logToLinearDepth(sampleDepth1)), 1.0));
  //float4 eyePosition2 = mul(u_inverseProjection, float4(input.edgeSampleUV2.x * 2.0 - 1.0, input.edgeSampleUV2.y * 2.0 - 1.0, linearDepthToClipZ(logToLinearDepth(sampleDepth2)), 1.0));
  //float4 eyePosition3 = mul(u_inverseProjection, float4(input.edgeSampleUV3.x * 2.0 - 1.0, input.edgeSampleUV3.y * 2.0 - 1.0, linearDepthToClipZ(logToLinearDepth(sampleDepth3)), 1.0));
  //
  //eyePosition0 /= eyePosition0.w;
  //eyePosition1 /= eyePosition1.w;
  //eyePosition2 /= eyePosition2.w;
  //eyePosition3 /= eyePosition3.w;
  //
  //float3 diff0 = eyePosition.xyz - eyePosition0.xyz;
  //float3 diff1 = eyePosition.xyz - eyePosition1.xyz;
  //float3 diff2 = eyePosition.xyz - eyePosition2.xyz;
  //float3 diff3 = eyePosition.xyz - eyePosition3.xyz;
  //
  //float isEdge = 1.0 - step(length(diff0), edgeOutlineThreshold) * step(length(diff1), edgeOutlineThreshold) * step(length(diff2), edgeOutlineThreshold) * step(length(diff3), edgeOutlineThreshold);
  //
  float3 edgeColour = lerp(col.xyz, u_outlineColour.xyz, u_outlineColour.w);
  float edgeLogDepth = logDepth;//min(min(min(sampleDepth0, sampleDepth1), sampleDepth2), sampleDepth3);
  return lerp(float4(col.xyz, logDepth), float4(edgeColour, edgeLogDepth), isEdge);
}

float3 hsv2rgb(float3 c)
{
  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// project position onto earth surface (in eye space)
// if result is negative, it is below earth surface
float calculateHeightAboveEarthSurface(float3 fragEyePosition)
{
  float3 eyeToEarthSurface = u_eyeToEarthSurfaceEyeSpace.xyz;
  float3 projectedPosition = (dot(fragEyePosition, eyeToEarthSurface) / dot(eyeToEarthSurface, eyeToEarthSurface)) * eyeToEarthSurface;
  
  float isBelowSurface = float(dot(projectedPosition, projectedPosition) < dot(eyeToEarthSurface, eyeToEarthSurface)) * 2.0 - 1.0;
  return length(eyeToEarthSurface - projectedPosition) * isBelowSurface;
}

float3 contourColour(float3 col, float3 fragEyePosition)
{
  float contourDistance = u_contourParams.x;
  float contourBandHeight = u_contourParams.y;
  float contourRainboxRepeat = u_contourParams.z;
  float contourRainboxIntensity = u_contourParams.w;

  float projectedHeight = abs(calculateHeightAboveEarthSurface(fragEyePosition));
  float3 rainbowColour = hsv2rgb(float3(projectedHeight * (1.0 / contourRainboxRepeat), 1.0, 1.0));
  float3 baseColour = lerp(col.xyz, rainbowColour, contourRainboxIntensity);

  float isContour = 1.0 - step(contourBandHeight, fmod(abs(projectedHeight), contourDistance));
  return lerp(baseColour, u_contourColour.xyz, isContour * u_contourColour.w);
}

float3 colourizeByHeight(float3 col, float3 fragEyePosition)
{
  float2 worldColourMinMax = u_colourizeHeightParams.xy;
  
  float projectedHeight = calculateHeightAboveEarthSurface(fragEyePosition);
  float minMaxColourStrength = getNormalizedPosition(projectedHeight, worldColourMinMax.x, worldColourMinMax.y);
  
  float3 minColour = lerp(col.xyz, u_colourizeHeightColourMin.xyz, u_colourizeHeightColourMin.w);
  float3 maxColour = lerp(col.xyz, u_colourizeHeightColourMax.xyz, u_colourizeHeightColourMax.w);
  return lerp(minColour, maxColour, minMaxColourStrength);
}

float3 colourizeByEyeDistance(float3 col, float3 fragEyePos)
{
  float2 depthColourMinMax = u_colourizeDepthParams.xy;

  float depthColourStrength = getNormalizedPosition(length(fragEyePos), depthColourMinMax.x, depthColourMinMax.y);
  return lerp(col.xyz, u_colourizeDepthColour.xyz, depthColourStrength * u_colourizeDepthColour.w);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 col = sceneColourTexture.Sample(sceneColourSampler, input.uv);
  float4 packedNormal = sceneNormalTexture.Sample(sceneNormalSampler, input.uv);
  float logDepth = sceneDepthTexture.Sample(sceneDepthSampler, input.uv).x;
	
  float depth = logToLinearDepth(logDepth);
  float clipZ = linearDepthToClipZ(depth);

  float4 fragEyePosition = mul(u_inverseProjection, float4(input.clip.xy, clipZ, 1.0));
  fragEyePosition /= fragEyePosition.w;

  col.xyz = colourizeByHeight(col.xyz, fragEyePosition.xyz);
  col.xyz = colourizeByEyeDistance(col.xyz, fragEyePosition.xyz);
  col.xyz = contourColour(col.xyz, fragEyePosition.xyz);
  
  float edgeOutlineWidth = u_outlineParams.x;
  float edgeOutlineThreshold = u_outlineParams.y;
  float4 outlineColour = u_outlineColour;
  if (edgeOutlineWidth > 0.0 && u_outlineColour.w > 0.0)
  {
    float4 edgeResult = edgeHighlight(input, col.xyz, depth, logDepth, outlineColour, edgeOutlineThreshold);
    col.xyz = edgeResult.xyz;
    logDepth = edgeResult.w; // to preserve outlines, depth written may be adjusted
  }

  output.Color0 = float4(col.xyz, 1.0);
  output.Depth0 = logDepth;
  
  output.Normal = packedNormal;
  return output;
}
