#include "gl/vcRenderShaders.h"
#include "udPlatformUtil.h"

const char *const g_VisualizationFragmentShader = R"shader(
 struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 edgeSampleUV0 : TEXCOORD1;
    float2 edgeSampleUV1 : TEXCOORD2;
    float2 edgeSampleUV2 : TEXCOORD3;
    float2 edgeSampleUV3 : TEXCOORD4;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
    float Depth0 : SV_Depth;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  cbuffer u_fragParams : register(b0)
  {
    float4 u_screenParams;  // sampleStepSizeX, sampleStepSizeY, near plane, far plane
    float4x4 u_inverseViewProjection;
    float4x4 u_inverseProjection;

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

  float linearizeDepth(float depth)
  {
    float nearPlane = u_screenParams.z;
    float farPlane = u_screenParams.w;

    return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
  }

  float getNormalizedPosition(float v, float min, float max)
  {
    return clamp((v - min) / (max - min), 0.0, 1.0);
  }

  // note: an adjusted depth is packed into the returned .w component
  // this is to show the edge highlights against the skybox
  float4 edgeHighlight(PS_INPUT input, float3 col, float depth, float4 outlineColour, float edgeOutlineThreshold)
  {
    float4 eyePosition = mul(u_inverseProjection, float4(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0, depth, 1.0));
    eyePosition /= eyePosition.w;

    float sampleDepth0 = texture1.Sample(sampler1, input.edgeSampleUV0).x;
    float sampleDepth1 = texture1.Sample(sampler1, input.edgeSampleUV1).x;
    float sampleDepth2 = texture1.Sample(sampler1, input.edgeSampleUV2).x;
    float sampleDepth3 = texture1.Sample(sampler1, input.edgeSampleUV3).x;

    float4 eyePosition0 = mul(u_inverseProjection, float4(input.edgeSampleUV0.x * 2.0 - 1.0, (1.0 - input.edgeSampleUV0.y) * 2.0 - 1.0, sampleDepth0, 1.0));
    float4 eyePosition1 = mul(u_inverseProjection, float4(input.edgeSampleUV1.x * 2.0 - 1.0, (1.0 - input.edgeSampleUV1.y) * 2.0 - 1.0, sampleDepth1, 1.0));
    float4 eyePosition2 = mul(u_inverseProjection, float4(input.edgeSampleUV2.x * 2.0 - 1.0, (1.0 - input.edgeSampleUV2.y) * 2.0 - 1.0, sampleDepth2, 1.0));
    float4 eyePosition3 = mul(u_inverseProjection, float4(input.edgeSampleUV3.x * 2.0 - 1.0, (1.0 - input.edgeSampleUV3.y) * 2.0 - 1.0, sampleDepth3, 1.0));
    
    eyePosition0 /= eyePosition0.w;
    eyePosition1 /= eyePosition1.w;
    eyePosition2 /= eyePosition2.w;
    eyePosition3 /= eyePosition3.w;
    
    float3 diff0 = eyePosition.xyz - eyePosition0.xyz;
    float3 diff1 = eyePosition.xyz - eyePosition1.xyz;
    float3 diff2 = eyePosition.xyz - eyePosition2.xyz;
    float3 diff3 = eyePosition.xyz - eyePosition3.xyz;

    float isEdge = 1.0 - step(length(diff0), edgeOutlineThreshold) * step(length(diff1), edgeOutlineThreshold) * step(length(diff2), edgeOutlineThreshold) * step(length(diff3), edgeOutlineThreshold);
    
    float3 edgeColour = lerp(col.xyz, u_outlineColour.xyz, u_outlineColour.w);
    float edgeDepth = min(min(min(sampleDepth0, sampleDepth1), sampleDepth2), sampleDepth3);
    return float4(lerp(col.xyz, edgeColour, isEdge), lerp(depth, edgeDepth, isEdge));
  }

  float3 hsv2rgb(float3 c)
  {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
  }

  float3 contourColour(float3 col, float3 fragWorldPosition)
  {
    float contourDistance = u_contourParams.x;
    float contourBandHeight = u_contourParams.y;
    float contourRainboxRepeat = u_contourParams.z;
    float contourRainboxIntensity = u_contourParams.w;

    float3 rainbowColour = hsv2rgb(float3(fragWorldPosition.z * (1.0 / contourRainboxRepeat), 1.0, 1.0));
    float3 baseColour = lerp(col.xyz, rainbowColour, contourRainboxIntensity);

    float isContour = 1.0 - step(contourBandHeight, fmod(abs(fragWorldPosition.z), contourDistance));
    return lerp(baseColour, u_contourColour.xyz, isContour * u_contourColour.w);
  }

  float3 colourizeByHeight(float3 col, float3 fragWorldPosition)
  {
    float2 worldColourMinMax = u_colourizeHeightParams.xy;

    float minMaxColourStrength = getNormalizedPosition(fragWorldPosition.z, worldColourMinMax.x, worldColourMinMax.y);

    float3 minColour = lerp(col.xyz, u_colourizeHeightColourMin.xyz, u_colourizeHeightColourMin.w);
    float3 maxColour = lerp( col.xyz, u_colourizeHeightColourMax.xyz,u_colourizeHeightColourMax.w);
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

    float4 col = texture0.Sample(sampler0, input.uv);
    float depth = texture1.Sample(sampler1, input.uv).x;

    // TODO: I'm fairly certain this is actually wrong (world space calculations), and will have precision issues
    float4 fragWorldPosition = mul(u_inverseViewProjection, float4(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0, depth, 1.0));
    fragWorldPosition /= fragWorldPosition.w;

    float4 fragEyePosition = mul(u_inverseProjection, float4(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0, depth, 1.0));
    fragEyePosition /= fragEyePosition.w;

    col.xyz = colourizeByHeight(col.xyz, fragWorldPosition.xyz);
    col.xyz = colourizeByEyeDistance(col.xyz, fragEyePosition.xyz);

    col.xyz = contourColour(col.xyz, fragWorldPosition.xyz);

    float edgeOutlineWidth = u_outlineParams.x;
    float edgeOutlineThreshold = u_outlineParams.y;
    float4 outlineColour = u_outlineColour;
    if (edgeOutlineWidth > 0.0 && u_outlineColour.w > 0.0)
    {
      float4 edgeResult = edgeHighlight(input, col.xyz, depth, outlineColour, edgeOutlineThreshold);
      col.xyz = edgeResult.xyz;
      depth = edgeResult.w; // to preserve outsides edges, depth written may be adjusted
    }

    output.Color0 = float4(col.xyz, 1.0);// UD always opaque
    output.Depth0 = depth;
    return output;
  }

)shader";

const char *const g_VisualizationVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 edgeSampleUV0 : TEXCOORD1;
    float2 edgeSampleUV1 : TEXCOORD2;
    float2 edgeSampleUV2 : TEXCOORD3;
    float2 edgeSampleUV3 : TEXCOORD4;
  };

  cbuffer u_vertParams : register(b1)
  {
    float4 u_outlineStepSize; // outlineStepSize.xy (in uv space), (unused), (unused)
  }

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;

    float3 sampleOffsets = float3(u_outlineStepSize.xy, 0.0);
    output.edgeSampleUV0 = output.uv + sampleOffsets.xz;
    output.edgeSampleUV1 = output.uv - sampleOffsets.xz;
    output.edgeSampleUV2 = output.uv + sampleOffsets.zy;
    output.edgeSampleUV3 = output.uv - sampleOffsets.zy;

    return output;
  }
)shader";

const char *const g_ViewShedFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  // Should match CPU
  #define MAP_COUNT 3

  cbuffer u_params : register(b0)
  {
    float4x4 u_shadowMapVP[MAP_COUNT];
    float4x4 u_inverseProjection;
    float4 u_visibleColour;
    float4 u_notVisibleColour;
    float4 u_nearFarPlane; // .zw unused
  };

  float linearizeDepth(float depth)
  {
    float nearPlane = u_nearFarPlane.x;
    float farPlane = u_nearFarPlane.y;
    return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
  }

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;

    float4 col = float4(0.0, 0.0, 0.0, 0.0);
    float depth = texture0.Sample(sampler0, input.uv).x;

    float4 fragEyePosition = mul(u_inverseProjection, float4(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0, depth, 1.0));
    fragEyePosition /= fragEyePosition.w;

    float3 sampleUV = float3(0, 0, 0);

    float bias = 0.000000425 * u_nearFarPlane.y;

    // unrolled loop
    float4 shadowMapCoord0 = mul(u_shadowMapVP[0], float4(fragEyePosition.xyz, 1.0));
    float4 shadowMapCoord1 = mul(u_shadowMapVP[1], float4(fragEyePosition.xyz, 1.0));
    float4 shadowMapCoord2 = mul(u_shadowMapVP[2], float4(fragEyePosition.xyz, 1.0));

    // bias z before w divide
    shadowMapCoord0.z -= bias;
    shadowMapCoord1.z -= bias;
    shadowMapCoord2.z -= bias;
    
    // note: z has no scale & biased because we are using a [0, 1] depth projection matrix here
    float3 shadowMapClip0 = (shadowMapCoord0.xyz / shadowMapCoord0.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);
    float3 shadowMapClip1 = (shadowMapCoord1.xyz / shadowMapCoord1.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);
    float3 shadowMapClip2 = (shadowMapCoord2.xyz / shadowMapCoord2.w) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0);
    
    float isInMap0 = float(shadowMapClip0.x >= 0.0 && shadowMapClip0.x <= 1.0 && shadowMapClip0.y >= 0.0 && shadowMapClip0.y <= 1.0 && shadowMapClip0.z >= 0.0 && shadowMapClip0.z <= 1.0);
    float isInMap1 = float(shadowMapClip1.x >= 0.0 && shadowMapClip1.x <= 1.0 && shadowMapClip1.y >= 0.0 && shadowMapClip1.y <= 1.0 && shadowMapClip1.z >= 0.0 && shadowMapClip1.z <= 1.0);
    float isInMap2 = float(shadowMapClip2.x >= 0.0 && shadowMapClip2.x <= 1.0 && shadowMapClip2.y >= 0.0 && shadowMapClip2.y <= 1.0 && shadowMapClip2.z >= 0.0 && shadowMapClip2.z <= 1.0);

    // atlas UVs
    float3 shadowMapUV0 = float3((0.0 / float(MAP_COUNT)) + shadowMapClip0.x / float(MAP_COUNT), 1.0 - shadowMapClip0.y, shadowMapClip0.z);
    float3 shadowMapUV1 = float3((1.0 / float(MAP_COUNT)) + shadowMapClip1.x / float(MAP_COUNT), 1.0 - shadowMapClip1.y, shadowMapClip1.z);
    float3 shadowMapUV2 = float3((2.0 / float(MAP_COUNT)) + shadowMapClip2.x / float(MAP_COUNT), 1.0 - shadowMapClip2.y, shadowMapClip2.z);

    sampleUV = lerp(sampleUV, shadowMapUV0, isInMap0);
    sampleUV = lerp(sampleUV, shadowMapUV1, isInMap1);
    sampleUV = lerp(sampleUV, shadowMapUV2, isInMap2);

    if (length(sampleUV) > 0.0)
    {
      // fragment is inside the view shed bounds
      float shadowMapDepth = texture1.Sample(sampler1, sampleUV.xy).x;
      float diff = (0.2 * u_nearFarPlane.y) * (linearizeDepth(sampleUV.z) - linearizeDepth(shadowMapDepth));
      col = lerp(u_visibleColour, u_notVisibleColour, clamp(diff, 0.0, 1.0));
    }

    output.Color0 = float4(col.xyz * col.w, 1.0); //additive
    return output;
  }

)shader";

const char *const g_ViewShedVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;
    return output;
  }
)shader";

const char *const g_udFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
    float Depth0 : SV_Depth;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;

    float4 col = texture0.Sample(sampler0, input.uv);
    float depth = texture1.Sample(sampler1, input.uv).x;

    output.Color0 = float4(col.xyz, 1.0);// UD always opaque
    output.Depth0 = depth;

    return output;
  }
)shader";

const char *const g_udVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;
    return output;
  }
)shader";


const char *const g_udSplatIdFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
    float Depth0 : SV_Depth;
  };

  cbuffer u_params : register(b0)
  {
    float4 u_idOverride;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  bool floatEquals(float a, float b)
  {
    return abs(a - b) <= 0.0015f;
  }

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;
    float4 col = texture0.Sample(sampler0, input.uv);
    float depth = texture1.Sample(sampler1, input.uv).x;

    output.Color0 = float4(0.0, 0.0, 0.0, 0.0);
    if ((u_idOverride.w == 0.0 || floatEquals(u_idOverride.w, col.w)))
    {
      output.Color0 = float4(col.w, 0, 0, 1.0);
    }

    output.Depth0 = depth;
    return output;
  }

)shader";

const char *const g_tileFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 col = texture0.Sample(sampler0, input.uv);
    return float4(col.xyz * input.colour.xyz, input.colour.w);
  }
)shader";

const char *const g_tileVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  // This should match CPU struct size
  #define VERTEX_COUNT 2

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_projection;
    float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
    float4 u_colour;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    // note: could have precision issues on some devices
    float4 finalClipPos = mul(u_projection, u_eyePositions[int(input.pos.z)]);

    output.colour = u_colour;
    output.uv = input.pos.xy;
    output.pos = finalClipPos;
    return output;
  }
)shader";

const char *const g_CompassFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float3 normal : COLOR0;
    float4 colour : COLOR1;
    float3 sunDirection : COLOR2;
    float4 fragClipPosition : COLOR3;
  };

  float4 main(PS_INPUT input) : SV_Target
  {
    float3 fakeEyeVector = normalize(input.fragClipPosition.xyz / input.fragClipPosition.w);
    float3 worldNormal = input.normal * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
    float ndotl = 0.5 + 0.5 * -dot(input.sunDirection, worldNormal);
    float edotr = max(0.0, -dot(-fakeEyeVector, worldNormal));
    edotr = pow(edotr, 60.0);
    float3 sheenColour = float3(1.0, 1.0, 0.9);
    return float4(input.colour.a * (ndotl * input.colour.xyz + edotr * sheenColour), 1.0);
  }
)shader";

const char *const g_CompassVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float3 normal : NORMAL;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float3 normal : COLOR0;
    float4 colour : COLOR1;
    float3 sunDirection : COLOR2;
    float4 fragClipPosition : COLOR3;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float3 u_sunDirection;
    float _padding;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
    output.normal = (input.normal * 0.5) + 0.5;
    output.colour = u_colour;
    output.sunDirection = u_sunDirection;
    output.fragClipPosition = output.pos;
    return output;
  }
  )shader";

const char *const g_vcSkyboxVertexShaderPanorama = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = float2(input.uv.x, 1.0 - input.uv.y);
    return output;
  }
)shader";

const char *const g_vcSkyboxFragmentShaderPanorama = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  cbuffer u_EveryFrame : register(b0)
  {
    float4x4 u_inverseViewProjection;
  };

  sampler sampler0;
  Texture2D u_texture;

  #define PI 3.14159265359

  float2 directionToLatLong(float3 dir)
  {
    float2 longlat = float2(atan2(dir.x, dir.y) + PI, acos(dir.z));
    return longlat / float2(2.0 * PI, PI);
  }

  float4 main(PS_INPUT input) : SV_Target
  {
    // work out 3D point
    float4 point3D = mul(u_inverseViewProjection, float4(input.uv * float2(2.0, 2.0) - float2(1.0, 1.0), 1.0, 1.0));
    point3D.xyz = normalize(point3D.xyz / point3D.w);
    return u_texture.Sample(sampler0, directionToLatLong(point3D.xyz));
  }
)shader";

const char *const g_vcSkyboxVertexShaderImageColour = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 tintColour : COLOR0;
  };

  cbuffer u_EveryFrame : register(b0)
  {
    float4 u_tintColour; //0 is full colour, 1 is full image
    float4 u_imageSize; //For purposes of tiling/stretching
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = float2(input.uv.x, 1.0 - input.uv.y) / u_imageSize.xy;
    output.tintColour = u_tintColour;
    return output;
  }
)shader";

const char *const g_vcSkyboxFragmentShaderImageColour = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 tintColour : COLOR0;
  };

  sampler sampler0;
  Texture2D u_texture;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 colour = u_texture.Sample(sampler0, input.uv).rgba;
    float effectiveAlpha = min(colour.a, input.tintColour.a);
    return float4((colour.rgb * effectiveAlpha) + (input.tintColour.rgb * (1 - effectiveAlpha)), 1);
  }
)shader";

const char *const g_ImGuiVertexShader = R"vert(
  cbuffer u_EveryFrame : register(b0)
  {
    float4x4 ProjectionMatrix;
  };

  struct VS_INPUT
  {
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
  }
)vert";

const char *const g_ImGuiFragmentShader = R"frag(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
  }
)frag";


const char *const g_FenceVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 ribbonInfo : COLOR0; // xyz: expand vector; z: pair id (0 or 1)
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  cbuffer u_EveryFrame : register(b0)
  {
    float4 u_bottomColour;
    float4 u_topColour;

    float u_orientation;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;

    float3 _padding;
  };

  cbuffer u_EveryObject : register(b1)
  {
    float4x4 u_worldViewProjectionMatrix;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    // fence horizontal UV pos packed into Y channel
    output.uv = float2(lerp(input.uv.y, input.uv.x, u_orientation) * u_textureRepeatScale - u_time * u_textureScrollSpeed, input.ribbonInfo.w);
    output.colour = lerp(u_bottomColour, u_topColour, input.ribbonInfo.w);

    float3 worldPosition = input.pos + lerp(float3(0, 0, input.ribbonInfo.w) * u_width, input.ribbonInfo.xyz, u_orientation);
    output.pos = mul(u_worldViewProjectionMatrix, float4(worldPosition, 1.0));
    return output;
  }
)shader";

const char *const g_FenceFragmentShader = R"shader(
  //Input Format
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 texCol = texture0.Sample(sampler0, input.uv);
    return float4(texCol.xyz * input.colour.xyz, texCol.w * input.colour.w);
  }
)shader";

const char *const g_WaterFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 fragEyePos : COLOR0;
    float4 colour : COLOR1;
  };

  cbuffer u_EveryFrameFrag : register(b0)
  {
    float4 u_specularDir;
    float4x4 u_eyeNormalMatrix;
    float4x4 u_inverseViewMatrix;
  };

  #define PI 3.14159265359

  float2 directionToLatLong(float3 dir)
  {
    float2 longlat = float2(atan2(dir.x, dir.y) + PI, acos(dir.z));
    return longlat / float2(2.0 * PI, PI);
  }

  sampler sampler0;
  sampler sampler1;
  Texture2D u_normalMap;
  Texture2D u_skybox;

  float4 main(PS_INPUT input) : SV_Target
  {
    float3 specularDir = normalize(u_specularDir.xyz);

    float3 normal0 = u_normalMap.Sample(sampler0, input.uv0).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
    float3 normal1 = u_normalMap.Sample(sampler0, input.uv1).xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
    float3 normal = normalize((normal0.xyz + normal1.xyz));

    float3 eyeToFrag = normalize(input.fragEyePos.xyz);
    float3 eyeSpecularDir = normalize(mul(u_eyeNormalMatrix, float4(specularDir, 0.0)).xyz);
    float3 eyeNormal = normalize(mul(u_eyeNormalMatrix, float4(normal, 0.0)).xyz);
    float3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));

    float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
    float nDotL = -dot(eyeNormal, eyeToFrag);
    float fresnel = nDotL * 0.5 + 0.5;

    float specular = pow(nDotS, 50.0) * 0.5;

    float3 deepFactor = float3(0.35, 0.35, 0.35);
    float3 shallowFactor = float3(1.0, 1.0, 0.6);

    float waterDepth = pow(max(0.0, dot(normal, float3(0.0, 0.0, 1.0))), 5.0); // guess 'depth' based on normal direction
    float3 refractionColour = input.colour.xyz * lerp(shallowFactor, deepFactor, waterDepth);

    // reflection
    float4 worldFragPos = mul(u_inverseViewMatrix, float4(eyeReflectionDir, 0.0));
    float4 skybox = u_skybox.Sample(sampler1, directionToLatLong(normalize(worldFragPos.xyz)));
    float3 reflectionColour = skybox.xyz;

    float3 finalColour = lerp(reflectionColour, refractionColour, fresnel * 0.75) + float3(specular, specular, specular);
    return float4(finalColour, 1.0);
  }
)shader";

const char *const g_WaterVertexShader = R"shader(
  struct VS_INPUT
  {
    float2 pos : POSITION;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0  : TEXCOORD0;
    float2 uv1  : TEXCOORD1;
    float4 fragEyePos : COLOR0;
    float3 colour : COLOR1;
  };

  cbuffer u_EveryFrameVert : register(b0)
  {
    float4 u_time;
  };

  cbuffer u_EveryObject : register(b1)
  {
    float4 u_colourAndSize;
    float4x4 u_worldViewMatrix;
    float4x4 u_worldViewProjectionMatrix;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    float uvScaleBodySize = u_colourAndSize.w; // packed here

    // scale the uvs with time
    float uvOffset = u_time.x * 0.0625;
    output.uv0 = uvScaleBodySize * input.pos.xy * float2(0.25, 0.25) - float2(uvOffset, uvOffset);
    output.uv1 = uvScaleBodySize * input.pos.yx * float2(0.50, 0.50) - float2(uvOffset, uvOffset * 0.75);

    output.fragEyePos = mul(u_worldViewMatrix, float4(input.pos, 0.0, 1.0));
    output.colour = u_colourAndSize.xyz;
    output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 0.0, 1.0));

    return output;
  }
)shader";

const char *const g_PolygonP3N3UV2FragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float4 colour : COLOR0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 col = texture0.Sample(sampler0, input.uv);
    float4 diffuseColour = col * input.colour;

    // some fixed lighting
    float3 lightDirection = normalize(float3(0.85, 0.15, 0.5));
    float ndotl = dot(input.normal, lightDirection) * 0.5 + 0.5;
    float3 diffuse = diffuseColour.xyz * ndotl;

    return float4(diffuse, diffuseColour.a);
  }
)shader";

const char *const g_PolygonP3N3UV2VertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float3 normal : NORMAL;
    //float4 colour : COLOR0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float3 normal : NORMAL;
    float4 colour : COLOR0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
    float4x4 u_worldMatrix;
    float4 u_colour;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    // making the assumption that the model matrix won't contain non-uniform scale
    float3 worldNormal = normalize(mul(u_worldMatrix, float4(input.normal, 0.0)).xyz);

    output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
    output.uv = input.uv;
    output.normal = worldNormal;
    output.colour = u_colour;// * input.colour;

    return output;
  }
)shader";

const char *const g_ImageRendererFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 colour : COLOR0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 col = texture0.Sample(sampler0, input.uv);
    return col * input.colour;
  }
)shader";

const char *const g_ImageRendererMeshVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float3 normal : NORMAL; // unused
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float4 colour : COLOR0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize; // unused
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
    output.uv = input.uv;
    output.colour = u_colour;

    return output;
  }
)shader";

const char *const g_ImageRendererBillboardVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float4 colour : COLOR0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.pos = mul(u_worldViewProjectionMatrix, float4(input.pos, 1.0));
    output.pos.xy += u_screenSize.z * output.pos.w * u_screenSize.xy * float2(input.uv.x * 2.0 - 1.0, input.uv.y * 2.0 - 1.0); // expand billboard

    output.uv = float2(input.uv.x, 1.0 - input.uv.y);
    output.colour = u_colour;

    return output;
  }
)shader";

const char *const g_FlatColour_FragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float3 normal : NORMAL;
    float4 colour : COLOR0;
  };

  float4 main(PS_INPUT input) : SV_Target
  {
    return input.colour;
  }
)shader";

const char *const g_DepthOnly_FragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
  };

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;

    output.Color0 = float4(0.0, 0.0, 0.0, 0.0);

    return output;
  }
)shader";

const char *const g_BlurVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0  : TEXCOORD0;
    float2 uv1  : TEXCOORD1;
    float2 uv2  : TEXCOORD2;
  };

  cbuffer u_EveryFrame : register(b0)
  {
    float4 u_stepSize; // remember: requires 16 byte alignment
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);

    // sample on edges, taking advantage of bilinear sampling
    float2 sampleOffset = 1.42 * u_stepSize.xy;
    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);
    output.uv0 = uv - sampleOffset;
    output.uv1 = uv;
    output.uv2 = uv + sampleOffset;

    return output;
  }
)shader";

const char *const g_BlurFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0  : TEXCOORD0;
    float2 uv1  : TEXCOORD1;
    float2 uv2  : TEXCOORD2;
  };

  sampler sampler0;
  Texture2D texture0;

  static float4 kernel[3] = {float4(0.0, 0.0, 0.0, 0.27901),
                              float4(1.0, 1.0, 1.0, 0.44198),
                              float4(0.0, 0.0, 0.0, 0.27901)};

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 colour = float4(0.0, 0.0, 0.0, 0.0);

    colour += kernel[0] * texture0.Sample(sampler0, input.uv0);
    colour += kernel[1] * texture0.Sample(sampler0, input.uv1);
    colour += kernel[2] * texture0.Sample(sampler0, input.uv2);

    return colour;
  }

)shader";

const char *const g_HighlightVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0  : TEXCOORD0;
    float2 uv1  : TEXCOORD1;
    float2 uv2  : TEXCOORD2;
    float2 uv3  : TEXCOORD3;
    float2 uv4  : TEXCOORD4;
    float4 colour : COLOR0;
    float4 stepSizeThickness : COLOR1;
  };

  static float2 searchKernel[4] = {float2(-1, -1), float2(1, -1), float2(-1,  1), float2(1,  1)};

  cbuffer u_EveryFrame : register(b0)
  {
    float4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
    float4 u_colour;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);
    output.colour = u_colour;
    output.stepSizeThickness = u_stepSizeThickness;

    output.uv0 = input.uv;
    output.uv1 = input.uv + u_stepSizeThickness.xy * searchKernel[0];
    output.uv2 = input.uv + u_stepSizeThickness.xy * searchKernel[1];
    output.uv3 = input.uv + u_stepSizeThickness.xy * searchKernel[2];
    output.uv4 = input.uv + u_stepSizeThickness.xy * searchKernel[3];

    return output;
  }
)shader";

const char *const g_HighlightFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv0  : TEXCOORD0;
    float2 uv1  : TEXCOORD1;
    float2 uv2  : TEXCOORD2;
    float2 uv3  : TEXCOORD3;
    float2 uv4  : TEXCOORD4;
    float4 colour : COLOR0;
    float4 stepSizeThickness : COLOR1;
  };

  sampler sampler0;
  Texture2D u_texture;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 middle = u_texture.Sample(sampler0, input.uv0);
    float result = middle.w;

    // 'outside' the geometry, just use the blurred 'distance'
    if (middle.x == 0.0)
      return float4(input.colour.xyz, result * input.stepSizeThickness.z * input.colour.a);

    result = 1.0 - result;

    // look for an edge, setting to full colour if found
    float softenEdge = 0.15 * input.colour.a;
    result += softenEdge * step(u_texture.Sample(sampler0, input.uv1).x - middle.x, -0.00001);
    result += softenEdge * step(u_texture.Sample(sampler0, input.uv2).x - middle.x, -0.00001);
    result += softenEdge * step(u_texture.Sample(sampler0, input.uv3).x - middle.x, -0.00001);
    result += softenEdge * step(u_texture.Sample(sampler0, input.uv4).x - middle.x, -0.00001);

    result = max(input.stepSizeThickness.w, result) * input.colour.w; // overlay colour
    return float4(input.colour.xyz, result);
  }

)shader";


const char *const g_udGPURenderQuadVertexShader = R"shader(
  struct VS_INPUT
  {
    float4 pos : POSITION;
    float4 colour  : COLOR0;
    float2 corner: TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    output.colour = input.colour.bgra;

    // Points
    float4 off = float4(input.pos.www * 2.0, 0);
    float4 pos0 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.www, 1.0));
    float4 pos1 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xww, 1.0));
    float4 pos2 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyw, 1.0));
    float4 pos3 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyw, 1.0));
    float4 pos4 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wwz, 1.0));
    float4 pos5 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xwz, 1.0));
    float4 pos6 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyz, 1.0));
    float4 pos7 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyz, 1.0));

    float4 minPos, maxPos;
    minPos = min(pos0, pos1);
    minPos = min(minPos, pos2);
    minPos = min(minPos, pos3);
    minPos = min(minPos, pos4);
    minPos = min(minPos, pos5);
    minPos = min(minPos, pos6);
    minPos = min(minPos, pos7);
    maxPos = max(pos0, pos1);
    maxPos = max(maxPos, pos2);
    maxPos = max(maxPos, pos3);
    maxPos = max(maxPos, pos4);
    maxPos = max(maxPos, pos5);
    maxPos = max(maxPos, pos6);
    maxPos = max(maxPos, pos7);
    output.pos = minPos + (maxPos - minPos) * 0.5;

    float2 pointSize = float2(maxPos.x - minPos.x, maxPos.y - minPos.y);
    output.pos.xy += pointSize * input.corner * float2(0.5, 0.5);
    return output;
  }
)shader";

const char *const g_udGPURenderQuadFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  float4 main(PS_INPUT input) : SV_Target
  {
    return input.colour;
  }
)shader";


const char *const g_udGPURenderGeomVertexShader = R"shader(
  struct VS_INPUT
  {
    float4 pos : POSITION;
    float4 colour  : COLOR0;
  };

  struct GS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 pointSize: TEXCOORD0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjectionMatrix;
    float4 u_colour;
  };

  GS_INPUT main(VS_INPUT input)
  {
    GS_INPUT output;

    output.colour = float4(input.colour.bgr * u_colour.xyz, u_colour.w);


    // Points
    float4 off = float4(input.pos.www * 2.0, 0);
    float4 pos0 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.www, 1.0));
    float4 pos1 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xww, 1.0));
    float4 pos2 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyw, 1.0));
    float4 pos3 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyw, 1.0));
    float4 pos4 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wwz, 1.0));
    float4 pos5 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xwz, 1.0));
    float4 pos6 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.xyz, 1.0));
    float4 pos7 = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz + off.wyz, 1.0));

    float4 minPos, maxPos;
    minPos = min(pos0, pos1);
    minPos = min(minPos, pos2);
    minPos = min(minPos, pos3);
    minPos = min(minPos, pos4);
    minPos = min(minPos, pos5);
    minPos = min(minPos, pos6);
    minPos = min(minPos, pos7);
    maxPos = max(pos0, pos1);
    maxPos = max(maxPos, pos2);
    maxPos = max(maxPos, pos3);
    maxPos = max(maxPos, pos4);
    maxPos = max(maxPos, pos5);
    maxPos = max(maxPos, pos6);
    maxPos = max(maxPos, pos7);
    output.pos = minPos + (maxPos - minPos) * 0.5;

    output.pointSize = float2(maxPos.x - minPos.x, maxPos.y - minPos.y);
    return output;
  }
)shader";

const char *const g_udGPURenderGeomFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  float4 main(PS_INPUT input) : SV_Target
  {
    return input.colour;
  }
)shader";

const char *const g_udGPURenderGeomGeometryShader = R"shader(
  struct GS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 pointSize: TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
  };

  [maxvertexcount(6)]
  void main(point GS_INPUT input[1], inout TriangleStream<PS_INPUT> OutputStream)
  {
    PS_INPUT output;
    output.colour = input[0].colour;

    float2 halfPointSize = input[0].pointSize * float2(0.5, 0.5);

    output.pos = input[0].pos + float4(-halfPointSize.x, -halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    output.pos = input[0].pos + float4(halfPointSize.x, -halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    output.pos = input[0].pos + float4(-halfPointSize.x, halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    //TODO: (EVC-720) Emit 4 verts as triangle strip
    //OutputStream.RestartStrip();

    output.pos = input[0].pos + float4(-halfPointSize.x, halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    output.pos = input[0].pos + float4(halfPointSize.x, halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    output.pos = input[0].pos + float4(halfPointSize.x, -halfPointSize.y, 0.0, 0.0);
    OutputStream.Append(output);

    //TODO: (EVC-720) Emit 4 verts as triangle strip
    //OutputStream.RestartStrip();
  }
)shader";

const char *const g_PostEffectsVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 edgeSampleUV0 : TEXCOORD1;
    float2 edgeSampleUV1 : TEXCOORD2;
    float2 edgeSampleUV2 : TEXCOORD3;
    float2 edgeSampleUV3 : TEXCOORD4;
    float2 sampleStepSize : TEXCOORD5;
    float saturation : TEXCOORD6;
  };

  cbuffer u_params : register(b0)
  {
    float4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, (unused), (unused)
    float4 u_saturation; // saturation, (unused), (unused), (unused)
  }

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;
    output.sampleStepSize = u_screenParams.xy;

    // sample corners
    output.edgeSampleUV0 = output.uv + u_screenParams.xy;
    output.edgeSampleUV1 = output.uv - u_screenParams.xy;
    output.edgeSampleUV2 = output.uv + float2(u_screenParams.x, -u_screenParams.y);
    output.edgeSampleUV3 = output.uv + float2(-u_screenParams.x, u_screenParams.y);

    output.saturation = u_saturation.x;

    return output;
  }
)shader";

const char *const g_PostEffectsFragmentShader = R"shader(

/*
============================================================================
                    NVIDIA FXAA 3.11 by TIMOTHY LOTTES
------------------------------------------------------------------------------
COPYRIGHT (C) 2010, 2011 NVIDIA CORPORATION. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL NVIDIA
OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR
LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION,
OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE
THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.
*/

#define FXAA_GREEN_AS_LUMA 1
#define FXAA_DISCARD 0
#define FXAA_FAST_PIXEL_OFFSET 0
#define FXAA_GATHER4_ALPHA 0

#define FXAA_QUALITY__PS 5
#define FXAA_QUALITY__P0 1.0
#define FXAA_QUALITY__P1 1.5
#define FXAA_QUALITY__P2 2.0
#define FXAA_QUALITY__P3 4.0
#define FXAA_QUALITY__P4 12.0

#define FxaaBool bool
#define FxaaDiscard clip(-1)
#define FxaaFloat float
#define FxaaFloat2 float2
#define FxaaFloat3 float3
#define FxaaFloat4 float4
#define FxaaHalf half
#define FxaaHalf2 half2
#define FxaaHalf3 half3
#define FxaaHalf4 half4
#define FxaaSat(x) saturate(x)

#define FxaaInt2 int2
struct FxaaTex { SamplerState smpl; Texture2D tex; };
#define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
#define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
#define FxaaTexAlpha4(t, p) t.tex.GatherAlpha(t.smpl, p)
#define FxaaTexOffAlpha4(t, p, o) t.tex.GatherAlpha(t.smpl, p, o)
#define FxaaTexGreen4(t, p) t.tex.GatherGreen(t.smpl, p)
#define FxaaTexOffGreen4(t, p, o) t.tex.GatherGreen(t.smpl, p, o)

FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.y; }  

FxaaFloat4 FxaaPixelShader(
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat4 fxaaConsole360ConstDir
) {
    FxaaFloat2 posM;
    posM.x = pos.x;
    posM.y = pos.y;

    FxaaFloat4 rgbyM = FxaaTexTop(tex, posM);

    #define lumaM rgbyM.y

    FxaaFloat lumaS = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 0, 1), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1, 0), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaN = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 0,-1), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 0), fxaaQualityRcpFrame.xy));

    FxaaFloat maxSM = max(lumaS, lumaM);
    FxaaFloat minSM = min(lumaS, lumaM);
    FxaaFloat maxESM = max(lumaE, maxSM);
    FxaaFloat minESM = min(lumaE, minSM);
    FxaaFloat maxWN = max(lumaN, lumaW);
    FxaaFloat minWN = min(lumaN, lumaW);
    FxaaFloat rangeMax = max(maxWN, maxESM);
    FxaaFloat rangeMin = min(minWN, minESM);
    FxaaFloat rangeMaxScaled = rangeMax * fxaaQualityEdgeThreshold;
    FxaaFloat range = rangeMax - rangeMin;
    FxaaFloat rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);
    FxaaBool earlyExit = range < rangeMaxClamped;
    if(earlyExit)
        #if (FXAA_DISCARD == 1)
            FxaaDiscard;
        #else
            return rgbyM;
        #endif

    FxaaFloat lumaNW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1,-1), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaSE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1, 1), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaNE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1,-1), fxaaQualityRcpFrame.xy));
    FxaaFloat lumaSW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 1), fxaaQualityRcpFrame.xy));

    FxaaFloat lumaNS = lumaN + lumaS;
    FxaaFloat lumaWE = lumaW + lumaE;
    FxaaFloat subpixRcpRange = 1.0/range;
    FxaaFloat subpixNSWE = lumaNS + lumaWE;
    FxaaFloat edgeHorz1 = (-2.0 * lumaM) + lumaNS;
    FxaaFloat edgeVert1 = (-2.0 * lumaM) + lumaWE;
    FxaaFloat lumaNESE = lumaNE + lumaSE;
    FxaaFloat lumaNWNE = lumaNW + lumaNE;
    FxaaFloat edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    FxaaFloat edgeVert2 = (-2.0 * lumaN) + lumaNWNE;
    FxaaFloat lumaNWSW = lumaNW + lumaSW;
    FxaaFloat lumaSWSE = lumaSW + lumaSE;
    FxaaFloat edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    FxaaFloat edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    FxaaFloat edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    FxaaFloat edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
    FxaaFloat edgeHorz = abs(edgeHorz3) + edgeHorz4;
    FxaaFloat edgeVert = abs(edgeVert3) + edgeVert4;
    FxaaFloat subpixNWSWNESE = lumaNWSW + lumaNESE;
    FxaaFloat lengthSign = fxaaQualityRcpFrame.x;
    FxaaBool horzSpan = edgeHorz >= edgeVert;
    FxaaFloat subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;
    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = fxaaQualityRcpFrame.y;
    FxaaFloat subpixB = (subpixA * (1.0/12.0)) - lumaM;
    FxaaFloat gradientN = lumaN - lumaM;
    FxaaFloat gradientS = lumaS - lumaM;
    FxaaFloat lumaNN = lumaN + lumaM;
    FxaaFloat lumaSS = lumaS + lumaM;
    FxaaBool pairN = abs(gradientN) >= abs(gradientS);
    FxaaFloat gradient = max(abs(gradientN), abs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    FxaaFloat subpixC = FxaaSat(abs(subpixB) * subpixRcpRange);
    FxaaFloat2 posB;
    posB.x = posM.x;
    posB.y = posM.y;
    FxaaFloat2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : fxaaQualityRcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : fxaaQualityRcpFrame.y;
    if(!horzSpan) posB.x += lengthSign * 0.5;
    if( horzSpan) posB.y += lengthSign * 0.5;
    FxaaFloat2 posN;
    posN.x = posB.x - offNP.x * FXAA_QUALITY__P0;
    posN.y = posB.y - offNP.y * FXAA_QUALITY__P0;
    FxaaFloat2 posP;
    posP.x = posB.x + offNP.x * FXAA_QUALITY__P0;
    posP.y = posB.y + offNP.y * FXAA_QUALITY__P0;
    FxaaFloat subpixD = ((-2.0)*subpixC) + 3.0;
    FxaaFloat lumaEndN = FxaaLuma(FxaaTexTop(tex, posN));
    FxaaFloat subpixE = subpixC * subpixC;
    FxaaFloat lumaEndP = FxaaLuma(FxaaTexTop(tex, posP));
    if(!pairN) lumaNN = lumaSS;
    FxaaFloat gradientScaled = gradient * 1.0/4.0;
    FxaaFloat lumaMM = lumaM - lumaNN * 0.5;
    FxaaFloat subpixF = subpixD * subpixE;
    FxaaBool lumaMLTZero = lumaMM < 0.0;
    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    FxaaBool doneN = abs(lumaEndN) >= gradientScaled;
    FxaaBool doneP = abs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P1;
    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P1;
    FxaaBool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P1;
    if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P1;
    if(doneNP) {
        if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P2;
        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P2;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P2;
        if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P2;
        if(doneNP) {
            if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
            if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P3;
            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P3;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P3;
            if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P3;
            if(doneNP) {
                if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P4;
                if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P4;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P4;
                if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P4;
             
            }
        }
    }
    FxaaFloat dstN = posM.x - posN.x;
    FxaaFloat dstP = posP.x - posM.x;
    if(!horzSpan) dstN = posM.y - posN.y;
    if(!horzSpan) dstP = posP.y - posM.y;
    FxaaBool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    FxaaFloat spanLength = (dstP + dstN);
    FxaaBool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    FxaaFloat spanLengthRcp = 1.0/spanLength;
    FxaaBool directionN = dstN < dstP;
    FxaaFloat dst = min(dstN, dstP);
    FxaaBool goodSpan = directionN ? goodSpanN : goodSpanP;
    FxaaFloat subpixG = subpixF * subpixF;
    FxaaFloat pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    FxaaFloat subpixH = subpixG * fxaaQualitySubpix;
    FxaaFloat pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    FxaaFloat pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;
    return FxaaFloat4(FxaaTexTop(tex, posM).xyz, lumaM);
}

  float3 saturation(float3 rgb, float adjustment)
  {
    const float3 W = float3(0.2125, 0.7154, 0.0721);
    float intensity = dot(rgb, W);
    return lerp(float3(intensity, intensity, intensity), rgb, adjustment);
  }

 struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 edgeSampleUV0 : TEXCOORD1;
    float2 edgeSampleUV1 : TEXCOORD2;
    float2 edgeSampleUV2 : TEXCOORD3;
    float2 edgeSampleUV3 : TEXCOORD4;
    float2 sampleStepSize : TEXCOORD5;
    float saturation : TEXCOORD6;
  };

  struct PS_OUTPUT
  {
    float4 Color0 : SV_Target;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;
    float4 colour = float4(0.0, 0.0, 0.0, 0.0);
    float depth = texture1.Sample(sampler1, input.uv).x;

    // only run FXAA on edges (simple edge detection)
    float depth0 = texture1.Sample(sampler1, input.edgeSampleUV0).x;
    float depth1 = texture1.Sample(sampler1, input.edgeSampleUV1).x;
    float depth2 = texture1.Sample(sampler1, input.edgeSampleUV2).x;
    float depth3 = texture1.Sample(sampler1, input.edgeSampleUV3).x;

    const float edgeThreshold = 0.003;
    float isEdge = 1.0 - (step(abs(depth0 - depth), edgeThreshold) * step(abs(depth1 - depth), edgeThreshold) * step(abs(depth2 - depth), edgeThreshold) * step(abs(depth3 - depth), edgeThreshold));
    if (isEdge == 0.0)
    {
      colour = texture0.Sample(sampler0, input.uv);
    }
    else
    {
      FxaaTex samplerInfo;
      samplerInfo.smpl = sampler0;
      samplerInfo.tex = texture0;
      
      colour = FxaaPixelShader(input.uv, float4(0, 0, 0, 0), samplerInfo, samplerInfo, samplerInfo, input.sampleStepSize,
                                     float4(0, 0, 0, 0), float4(0, 0, 0, 0), float4(0, 0, 0, 0),
                                     0.75,  //fxaaQualitySubpix
                                     0.125, // fxaaQualityEdgeThreshold
                                     0.0, // fxaaQualityEdgeThresholdMin
                                     0, 0, float4(0, 0, 0, 0));
    }
 
    output.Color0 = float4(saturation(colour.xyz, input.saturation), 1.0);
    return output;
  }
)shader";
