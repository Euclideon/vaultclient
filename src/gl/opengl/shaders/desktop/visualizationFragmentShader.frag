#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

in vec2 v_uv;
in vec2 v_edgeSampleUV0;
in vec2 v_edgeSampleUV1;
in vec2 v_edgeSampleUV2;
in vec2 v_edgeSampleUV3;

out vec4 out_Colour;

uniform sampler2D u_texture;
uniform sampler2D u_depth;

layout (std140) uniform u_fragParams
{
  vec4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, (unused), (unused)
  mat4 u_inverseViewProjection;
  mat4 u_inverseProjection;

  // outlining
  vec4 u_outlineColour;
  vec4 u_outlineParams;   // outlineWidth, edge threshold, (unused), (unused)

  // colour by height
  vec4 u_colourizeHeightColourMin;
  vec4 u_colourizeHeightColourMax;
  vec4 u_colourizeHeightParams; // min world height, max world height, (unused), (unused)

  // colour by depth
  vec4 u_colourizeDepthColour;
  vec4 u_colourizeDepthParams; // min distance, max distance, (unused), (unused)

  // contours
  vec4 u_contourColour;
  vec4 u_contourParams; // contour distance, contour band height, contour rainbow repeat rate, contour rainbow factoring
};

float logToLinearDepth(float logDepth)
{
  float a = s_CameraFarPlane / (s_CameraFarPlane - s_CameraNearPlane);
  float b = s_CameraFarPlane * s_CameraNearPlane / (s_CameraNearPlane - s_CameraFarPlane);
  float worldDepth = pow(2.0, logDepth * log2(s_CameraFarPlane + 1.0)) - 1.0;
  return a + b / worldDepth;
}

float getNormalizedPosition(float v, float min, float max)
{
  return clamp((v - min) / (max - min), 0.0, 1.0);
}

// note: an adjusted depth is packed into the returned .w component
// this is to show the edge highlights against the skybox
vec4 edgeHighlight(vec3 col, vec2 uv, float depth, float logDepth, vec4 outlineColour, float edgeOutlineThreshold)
{
  vec4 eyePosition = u_inverseProjection * vec4(uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  eyePosition /= eyePosition.w;

  float sampleDepth0 = texture(u_depth, v_edgeSampleUV0).x;
  float sampleDepth1 = texture(u_depth, v_edgeSampleUV1).x;
  float sampleDepth2 = texture(u_depth, v_edgeSampleUV2).x;
  float sampleDepth3 = texture(u_depth, v_edgeSampleUV3).x;

  vec4 eyePosition0 = u_inverseProjection * vec4(v_edgeSampleUV0 * vec2(2.0) - vec2(1.0), logToLinearDepth(sampleDepth0) * 2.0 - 1.0, 1.0);
  vec4 eyePosition1 = u_inverseProjection * vec4(v_edgeSampleUV1 * vec2(2.0) - vec2(1.0), logToLinearDepth(sampleDepth1) * 2.0 - 1.0, 1.0);
  vec4 eyePosition2 = u_inverseProjection * vec4(v_edgeSampleUV2 * vec2(2.0) - vec2(1.0), logToLinearDepth(sampleDepth2) * 2.0 - 1.0, 1.0);
  vec4 eyePosition3 = u_inverseProjection * vec4(v_edgeSampleUV3 * vec2(2.0) - vec2(1.0), logToLinearDepth(sampleDepth3) * 2.0 - 1.0, 1.0);

  eyePosition0 /= eyePosition0.w;
  eyePosition1 /= eyePosition1.w;
  eyePosition2 /= eyePosition2.w;
  eyePosition3 /= eyePosition3.w;

  vec3 diff0 = eyePosition.xyz - eyePosition0.xyz;
  vec3 diff1 = eyePosition.xyz - eyePosition1.xyz;
  vec3 diff2 = eyePosition.xyz - eyePosition2.xyz;
  vec3 diff3 = eyePosition.xyz - eyePosition3.xyz;

  float isEdge = 1.0 - step(length(diff0), edgeOutlineThreshold) * step(length(diff1), edgeOutlineThreshold) * step(length(diff2), edgeOutlineThreshold) * step(length(diff3), edgeOutlineThreshold);

  vec3 edgeColour = mix(col.xyz, outlineColour.xyz, outlineColour.w);
  float edgeLogDepth = min(min(min(sampleDepth0, sampleDepth1), sampleDepth2), sampleDepth3);
  return mix(vec4(col.xyz, logDepth), vec4(edgeColour, edgeLogDepth), isEdge);
}

vec3 hsv2rgb(vec3 c)
{
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 contourColour(vec3 col, vec3 fragWorldPosition)
{
  float contourDistance = u_contourParams.x;
  float contourBandHeight = u_contourParams.y;
  float contourRainboxRepeat = u_contourParams.z;
  float contourRainboxIntensity = u_contourParams.w;

  vec3 rainbowColour = hsv2rgb(vec3(fragWorldPosition.z * (1.0 / contourRainboxRepeat), 1.0, 1.0));
  vec3 baseColour = mix(col.xyz, rainbowColour, contourRainboxIntensity);

  float isContour = 1.0 - step(contourBandHeight, mod(abs(fragWorldPosition.z), contourDistance));
  return mix(baseColour, u_contourColour.xyz, isContour * u_contourColour.w);
}

vec3 colourizeByHeight(vec3 col, vec3 fragWorldPosition)
{
  vec2 worldColourMinMax = u_colourizeHeightParams.xy;

  float minMaxColourStrength = getNormalizedPosition(fragWorldPosition.z, worldColourMinMax.x, worldColourMinMax.y);

  vec3 minColour = mix(col.xyz, u_colourizeHeightColourMin.xyz, u_colourizeHeightColourMin.w);
  vec3 maxColour = mix( col.xyz, u_colourizeHeightColourMax.xyz,u_colourizeHeightColourMax.w);
  return mix(minColour, maxColour, minMaxColourStrength);
}

vec3 colourizeByEyeDistance(vec3 col, vec3 fragEyePos)
{
  vec2 depthColourMinMax = u_colourizeDepthParams.xy;

  float depthColourStrength = getNormalizedPosition(length(fragEyePos), depthColourMinMax.x, depthColourMinMax.y);
  return mix(col.xyz, u_colourizeDepthColour.xyz, depthColourStrength * u_colourizeDepthColour.w);
}

void main()
{
  vec4 col = texture(u_texture, v_uv);
  float logDepth = texture(u_depth, v_uv).x; 
  float depth = logToLinearDepth(logDepth);

  // TODO: I'm fairly certain this is actually wrong (world space calculations), and will have precision issues
  vec4 fragWorldPosition = u_inverseViewProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  fragWorldPosition /= fragWorldPosition.w;

  vec4 fragEyePosition = u_inverseProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  fragEyePosition /= fragEyePosition.w;
  
  col.xyz = colourizeByHeight(col.xyz, fragWorldPosition.xyz);
  col.xyz = colourizeByEyeDistance(col.xyz, fragEyePosition.xyz);
  
  col.xyz = contourColour(col.xyz, fragWorldPosition.xyz);

  float edgeOutlineWidth = u_outlineParams.x;
  float edgeOutlineThreshold = u_outlineParams.y;
  vec4 outlineColour = u_outlineColour;
  if (outlineColour.w > 0.0 && edgeOutlineWidth > 0.0 && u_outlineColour.w > 0.0)
  {
    vec4 edgeResult = edgeHighlight(col.xyz, v_uv, depth, logDepth, outlineColour, edgeOutlineThreshold);
    col.xyz = edgeResult.xyz;
    logDepth = edgeResult.w; // to preserve outlines, depth written may be adjusted
  }

  out_Colour = vec4(col.xyz, 1.0);
  gl_FragDepth = logDepth;
}
