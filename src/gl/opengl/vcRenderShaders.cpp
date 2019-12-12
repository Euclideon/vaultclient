#include "gl/vcRenderShaders.h"
#include "udPlatformUtil.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN || UDPLATFORM_ANDROID
# define FRAG_HEADER "#version 300 es\nprecision highp float;\n"
# define VERT_HEADER "#version 300 es\n"
#else
# define FRAG_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\n"
# define VERT_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\n"
#endif

const char *const g_VisualizationFragmentShader = FRAG_HEADER R"shader(
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
  vec4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, near plane, far plane
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
vec4 edgeHighlight(vec3 col, vec2 uv, float depth, vec4 outlineColour, float edgeOutlineThreshold)
{
  vec4 eyePosition = u_inverseProjection * vec4(uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  eyePosition /= eyePosition.w;

  float sampleDepth0 = texture(u_depth, v_edgeSampleUV0).x;
  float sampleDepth1 = texture(u_depth, v_edgeSampleUV1).x;
  float sampleDepth2 = texture(u_depth, v_edgeSampleUV2).x;
  float sampleDepth3 = texture(u_depth, v_edgeSampleUV3).x;

  vec4 eyePosition0 = u_inverseProjection * vec4(v_edgeSampleUV0 * vec2(2.0) - vec2(1.0), sampleDepth0 * 2.0 - 1.0, 1.0);
  vec4 eyePosition1 = u_inverseProjection * vec4(v_edgeSampleUV1 * vec2(2.0) - vec2(1.0), sampleDepth1 * 2.0 - 1.0, 1.0);
  vec4 eyePosition2 = u_inverseProjection * vec4(v_edgeSampleUV2 * vec2(2.0) - vec2(1.0), sampleDepth2 * 2.0 - 1.0, 1.0);
  vec4 eyePosition3 = u_inverseProjection * vec4(v_edgeSampleUV3 * vec2(2.0) - vec2(1.0), sampleDepth3 * 2.0 - 1.0, 1.0);

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
  float edgeDepth = min(min(min(sampleDepth0, sampleDepth1), sampleDepth2), sampleDepth3);
  return vec4(mix(col.xyz, edgeColour, isEdge), mix(depth, edgeDepth, isEdge));
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
  float depth = texture(u_depth, v_uv).x;

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
    vec4 edgeResult = edgeHighlight(col.xyz, v_uv, depth, outlineColour, edgeOutlineThreshold);
    col.xyz = edgeResult.xyz;
    depth = edgeResult.w; // to preserve outsides edges, depth written may be adjusted
  }

  out_Colour = vec4(col.xyz, 1.0);
  gl_FragDepth = depth;
}

)shader";

const char *const g_VisualizationVertexShader = FRAG_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;
out vec2 v_edgeSampleUV0;
out vec2 v_edgeSampleUV1;
out vec2 v_edgeSampleUV2;
out vec2 v_edgeSampleUV3;

layout (std140) uniform u_vertParams
{
  vec4 u_outlineStepSize; // outlineStepSize.xy (in uv space), (unused), (unused)
};

void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);

  vec3 sampleOffsets = vec3(u_outlineStepSize.xy, 0.0);
  v_edgeSampleUV0 = v_uv + sampleOffsets.xz;
  v_edgeSampleUV1 = v_uv - sampleOffsets.xz;
  v_edgeSampleUV2 = v_uv + sampleOffsets.zy;
  v_edgeSampleUV3 = v_uv - sampleOffsets.zy;
}
)shader";

const char *const g_ViewShedFragmentShader = FRAG_HEADER R"shader(
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
  vec4 u_nearFarPlane; // .zw unused
};

float linearizeDepth(float depth)
{
  float nearPlane = u_nearFarPlane.x;
  float farPlane = u_nearFarPlane.y;
  return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
}

void main()
{
  vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
  float depth = texture(u_depth, v_uv).x;

  vec4 fragEyePosition = u_inverseProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  fragEyePosition /= fragEyePosition.w;

  vec3 sampleUV = vec3(0.0);

  float bias = 0.000000425 * u_nearFarPlane.y;

  // unrolled loop
  vec4 shadowMapCoord0 = u_shadowMapVP[0] * vec4(fragEyePosition.xyz, 1.0);
  vec4 shadowMapCoord1 = u_shadowMapVP[1] * vec4(fragEyePosition.xyz, 1.0);
  vec4 shadowMapCoord2 = u_shadowMapVP[2] * vec4(fragEyePosition.xyz, 1.0);

  // bias z before w divide
  shadowMapCoord0.z -= bias;
  shadowMapCoord1.z -= bias;
  shadowMapCoord2.z -= bias;

  // note: z has no scale & biased because we are using a [0,1] depth projection matrix here
  vec3 shadowMapClip0 = (shadowMapCoord0.xyz / shadowMapCoord0.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
  vec3 shadowMapClip1 = (shadowMapCoord1.xyz / shadowMapCoord1.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
  vec3 shadowMapClip2 = (shadowMapCoord2.xyz / shadowMapCoord2.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);

  float isInMap0 = float(shadowMapClip0.x >= 0.0 && shadowMapClip0.x <= 1.0 && shadowMapClip0.y >= 0.0 && shadowMapClip0.y <= 1.0 && shadowMapClip0.z >= 0.0 && shadowMapClip0.z <= 1.0);
  float isInMap1 = float(shadowMapClip1.x >= 0.0 && shadowMapClip1.x <= 1.0 && shadowMapClip1.y >= 0.0 && shadowMapClip1.y <= 1.0 && shadowMapClip1.z >= 0.0 && shadowMapClip1.z <= 1.0);
  float isInMap2 = float(shadowMapClip2.x >= 0.0 && shadowMapClip2.x <= 1.0 && shadowMapClip2.y >= 0.0 && shadowMapClip2.y <= 1.0 && shadowMapClip2.z >= 0.0 && shadowMapClip2.z <= 1.0);

  // atlas UVs
  vec3 shadowMapUV0 = vec3((0.0 / float(MAP_COUNT)) + shadowMapClip0.x / float(MAP_COUNT), shadowMapClip0.y, shadowMapClip0.z);
  vec3 shadowMapUV1 = vec3((1.0 / float(MAP_COUNT)) + shadowMapClip1.x / float(MAP_COUNT), shadowMapClip1.y, shadowMapClip1.z);
  vec3 shadowMapUV2 = vec3((2.0 / float(MAP_COUNT)) + shadowMapClip2.x / float(MAP_COUNT), shadowMapClip2.y, shadowMapClip2.z);

  sampleUV = mix(sampleUV, shadowMapUV0, isInMap0);
  sampleUV = mix(sampleUV, shadowMapUV1, isInMap1);
  sampleUV = mix(sampleUV, shadowMapUV2, isInMap2);

  if (length(sampleUV) > 0.0)
  {
    float shadowMapDepth = texture(u_shadowMapAtlas, sampleUV.xy).x;
    float diff = (0.2 * u_nearFarPlane.y) * (linearizeDepth(sampleUV.z) - linearizeDepth(shadowMapDepth));
    col = mix(u_visibleColour, u_notVisibleColour, clamp(diff, 0.0, 1.0));
  }

  out_Colour = vec4(col.xyz * col.w, 1.0); //additive
}

)shader";

const char *const g_ViewShedVertexShader = FRAG_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;

void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
}
)shader";

const char *const g_udFragmentShader = FRAG_HEADER R"shader(
//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;
uniform sampler2D u_depth;

void main()
{
)shader"

#if UDPLATFORM_EMSCRIPTEN
"  vec4 col = texture(u_texture, v_uv).bgra;"
#else
"  vec4 col = texture(u_texture, v_uv);"
#endif

R"shader(

  float depth = texture(u_depth, v_uv).x;

  out_Colour = vec4(col.xyz, 1.0); // UD always opaque
  gl_FragDepth = depth;
}
)shader";

const char *const g_udSplatIdFragmentShader = FRAG_HEADER R"shader(
//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

layout (std140) uniform u_params
{
  vec4 u_idOverride;
};

uniform sampler2D u_texture;
uniform sampler2D u_depth;

bool floatEquals(float a, float b)
{
  return abs(a - b) <= 0.0015f;
}

void main()
{
  gl_FragDepth = texture(u_depth, v_uv).x;
  out_Colour = vec4(0.0);

  vec4 col = texture(u_texture, v_uv);
  if (u_idOverride.w == 0.0 || floatEquals(u_idOverride.w, col.w))
  {
    out_Colour = vec4(col.w, 0, 0, 1.0);
  }
}
)shader";

const char *const g_udVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;

void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = a_texCoord;
}
)shader";

const char *const g_tileFragmentShader = FRAG_HEADER R"shader(
//Input Format
in vec4 v_colour;
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  out_Colour = vec4(col.xyz * v_colour.xyz, v_colour.w);
}
)shader";

const char *const g_tileVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_uv;

//Output Format
out vec4 v_colour;
out vec2 v_uv;

// This should match CPU struct size
#define VERTEX_COUNT 2

layout (std140) uniform u_EveryObject
{
  mat4 u_projection;
  vec4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
  vec4 u_colour;
};

void main()
{
  // TODO: could have precision issues on some devices
  vec4 finalClipPos = u_projection * u_eyePositions[int(a_uv.z)];

  v_uv = a_uv.xy;
  v_colour = u_colour;
  gl_Position = finalClipPos;
}
)shader";


const char *const g_vcSkyboxVertexShaderPanorama = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
}
)shader";

const char *const g_vcSkyboxFragmentShaderPanorama = FRAG_HEADER R"shader(
uniform sampler2D u_texture;
layout (std140) uniform u_EveryFrame
{
  mat4 u_inverseViewProjection;
};

//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

#define PI 3.14159265359

vec2 directionToLatLong(vec3 dir)
{
  vec2 longlat = vec2(atan(dir.x, dir.y) + PI, acos(dir.z));
  return longlat / vec2(2.0 * PI, PI);
}

void main()
{
  // work out 3D point
  vec4 point3D = u_inverseViewProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), 1.0, 1.0);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  vec4 c1 = texture(u_texture, directionToLatLong(point3D.xyz));

  out_Colour = c1;
}
)shader";

const char *const g_vcSkyboxVertexShaderImageColour = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;
out vec4 v_tintColour;

layout (std140) uniform u_EveryFrame
{
  vec4 u_tintColour; //0 is full colour, 1 is full image
  vec4 u_imageSize; //For purposes of tiling/stretching
};

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y) / u_imageSize.xy;
  v_tintColour = u_tintColour;
}
)shader";

const char *const g_vcSkyboxFragmentShaderImageColour = FRAG_HEADER R"shader(
uniform sampler2D u_texture;

//Input Format
in vec2 v_uv;
in vec4 v_tintColour;

//Output Format
out vec4 out_Colour;

void main()
{
  vec4 colour = texture(u_texture, v_uv).rgba;
  float effectiveAlpha = min(colour.a, v_tintColour.a);
  out_Colour = vec4((colour.rgb * effectiveAlpha) + (v_tintColour.rgb * (1.0 - effectiveAlpha)), 1);
}
)shader";


const char *const g_CompassFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec4 v_colour;
  in vec3 v_normal;
  in vec4 v_fragClipPosition;
  in vec3 v_sunDirection;

  //Output Format
  out vec4 out_Colour;

  void main()
  {
    // fake a reflection vector
    vec3 fakeEyeVector = normalize(v_fragClipPosition.xyz / v_fragClipPosition.w);
    vec3 worldNormal = normalize(v_normal * vec3(2.0) - vec3(1.0));
    float ndotl = 0.5 + 0.5 * -dot(v_sunDirection, worldNormal);
    float edotr = max(0.0, -dot(-fakeEyeVector, worldNormal));
    edotr = pow(edotr, 60.0);
    vec3 sheenColour = vec3(1.0, 1.0, 0.9);
    out_Colour = vec4(v_colour.a * (ndotl * v_colour.xyz + edotr * sheenColour), 1.0);
  }
)shader";

const char *const g_CompassVertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec3 a_normal;

  //Output Format
  out vec4 v_colour;
  out vec3 v_normal;
  out vec4 v_fragClipPosition;
  out vec3 v_sunDirection;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec3 u_sunDirection;
    float _padding;
  };

  void main()
  {
    gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);

    v_normal = ((a_normal * 0.5) + 0.5);
    v_colour = u_colour;
    v_sunDirection = u_sunDirection;
    v_fragClipPosition = gl_Position;
  }
)shader";

const char *const g_ImGuiVertexShader = VERT_HEADER R"shader(
layout (std140) uniform u_EveryFrame
{
  mat4 ProjMtx;
};

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 Color;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
}
)shader";

const char *const g_ImGuiFragmentShader = FRAG_HEADER R"shader(
uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main()
{
  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
}
)shader";

const char *const g_FenceVertexShader = VERT_HEADER R"shader(
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_ribbonInfo; // xyz: expand vector; z: pair id (0 or 1)

out vec2 v_uv;
out vec4 v_colour;

layout (std140) uniform u_EveryFrame
{
  vec4 u_bottomColour;
  vec4 u_topColour;

  float u_orientation;
  float u_width;
  float u_textureRepeatScale;
  float u_textureScrollSpeed;
  float u_time;

  vec3 _padding;
};

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
};

void main()
{
  // fence horizontal UV pos packed into Y channel
  v_uv = vec2(mix(a_uv.y, a_uv.x, u_orientation) * u_textureRepeatScale - u_time * u_textureScrollSpeed, a_ribbonInfo.w);
  v_colour = mix(u_bottomColour, u_topColour, a_ribbonInfo.w);

  // fence or flat
  vec3 worldPosition = a_position + mix(vec3(0, 0, a_ribbonInfo.w) * u_width, a_ribbonInfo.xyz, u_orientation);

  gl_Position = u_worldViewProjectionMatrix * vec4(worldPosition, 1.0);
}
)shader";

const char *const g_FenceFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv;
  in vec4 v_colour;

  //Output Format
  out vec4 out_Colour;

  uniform sampler2D u_texture;

  void main()
  {
    vec4 texCol = texture(u_texture, v_uv);
    out_Colour = vec4(texCol.xyz * v_colour.xyz, texCol.w * v_colour.w);
  }
)shader";

const char *const g_WaterFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv0;
  in vec2 v_uv1;
  in vec4 v_fragEyePos;
  in vec3 v_colour;

  //Output Format
  out vec4 out_Colour;

  layout (std140) uniform u_EveryFrameFrag
  {
    vec4 u_specularDir;
    mat4 u_eyeNormalMatrix;
    mat4 u_inverseViewMatrix;
  };

  uniform sampler2D u_normalMap;
  uniform sampler2D u_skybox;

  #define PI 3.14159265359

  vec2 directionToLatLong(vec3 dir)
  {
    vec2 longlat = vec2(atan(dir.x, dir.y) + PI, acos(dir.z));
    return longlat / vec2(2.0 * PI, PI);
  }

  void main()
  {
    vec3 specularDir = normalize(u_specularDir.xyz);

    vec3 normal0 = texture(u_normalMap, v_uv0).xyz * vec3(2.0) - vec3(1.0);
    vec3 normal1 = texture(u_normalMap, v_uv1).xyz * vec3(2.0) - vec3(1.0);
    vec3 normal = normalize((normal0.xyz + normal1.xyz));

    vec3 eyeToFrag = normalize(v_fragEyePos.xyz);
    vec3 eyeSpecularDir = normalize((u_eyeNormalMatrix * vec4(specularDir, 0.0)).xyz);
    vec3 eyeNormal = normalize((u_eyeNormalMatrix * vec4(normal, 0.0)).xyz);
    vec3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));

    float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
    float nDotL = -dot(eyeNormal, eyeToFrag);
    float fresnel = nDotL * 0.5 + 0.5;

    float specular = pow(nDotS, 50.0) * 0.5;

    vec3 deepFactor = vec3(0.35, 0.35, 0.35);
    vec3 shallowFactor = vec3(1.0, 1.0, 0.6);

    float waterDepth = pow(max(0.0, dot(normal, vec3(0.0, 0.0, 1.0))), 5.0); // guess 'depth' based on normal direction
    vec3 refractionColour = v_colour.xyz * mix(shallowFactor, deepFactor, waterDepth);

    // reflection
    vec4 worldFragPos = u_inverseViewMatrix * vec4(eyeReflectionDir, 0.0);
    vec4 skybox = texture(u_skybox, directionToLatLong(normalize(worldFragPos.xyz)));
    vec3 reflectionColour = skybox.xyz;

    vec3 finalColour = mix(reflectionColour, refractionColour, fresnel * 0.75) + vec3(specular);
    out_Colour = vec4(finalColour, 1.0);
  }
)shader";

const char *const g_WaterVertexShader = VERT_HEADER R"shader(
  layout(location = 0) in vec2 a_position;

  out vec2 v_uv0;
  out vec2 v_uv1;
  out vec4 v_fragEyePos;
  out vec3 v_colour;

  layout (std140) uniform u_EveryFrameVert
  {
    vec4 u_time;
  };

  layout (std140) uniform u_EveryObject
  {
    vec4 u_colourAndSize;
    mat4 u_worldViewMatrix;
    mat4 u_worldViewProjectionMatrix;
  };

  void main()
  {
    float uvScaleBodySize = u_colourAndSize.w; // packed here

    // scale the uvs with time
    float uvOffset = u_time.x * 0.0625;
    v_uv0 = uvScaleBodySize * a_position.xy * vec2(0.25) - vec2(uvOffset, uvOffset);
    v_uv1 = uvScaleBodySize * a_position.yx * vec2(0.50) - vec2(uvOffset, uvOffset * 0.75);

    v_fragEyePos = u_worldViewMatrix * vec4(a_position, 0.0, 1.0);
    v_colour = u_colourAndSize.xyz;

    gl_Position = u_worldViewProjectionMatrix * vec4(a_position, 0.0, 1.0);
  }
)shader";

const char *const g_PolygonP3N3UV2FragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv;
  in vec4 v_colour;
  in vec3 v_normal;

  //Output Format
  out vec4 out_Colour;

  uniform sampler2D u_texture;

  void main()
  {
    vec4 col = texture(u_texture, v_uv);
    vec4 diffuseColour = col * v_colour;

    // some fixed lighting
    vec3 lightDirection = normalize(vec3(0.85, 0.15, 0.5));
    float ndotl = dot(v_normal, lightDirection) * 0.5 + 0.5;
    vec3 diffuse = diffuseColour.xyz * ndotl;

    out_Colour = vec4(diffuse, diffuseColour.a);
  }
)shader";

const char *const g_PolygonP3N3UV2VertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec3 a_normal;
  layout(location = 2) in vec2 a_uv;
  //layout(location = 3) in vec4 a_colour;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;
  out vec3 v_normal;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
    mat4 u_worldMatrix;
    vec4 u_colour;
  };

  void main()
  {
    // making the assumption that the model matrix won't contain non-uniform scale
    vec3 worldNormal = normalize((u_worldMatrix * vec4(a_normal, 0.0)).xyz);

    gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);

    v_uv = a_uv;
    v_normal = worldNormal;
    v_colour = u_colour;// * a_colour;
  }
)shader";

const char *const g_ImageRendererFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv;
  in vec4 v_colour;

  //Output Format
  out vec4 out_Colour;

  uniform sampler2D u_texture;

  void main()
  {
    vec4 col = texture(u_texture, v_uv);
    out_Colour = col * v_colour;
  }
)shader";

const char *const g_ImageRendererMeshVertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec3 a_normal; //unused
  layout(location = 2) in vec2 a_uv;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize; // unused
  };

  void main()
  {
    gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);

    v_uv = a_uv;
    v_colour = u_colour;
  }
)shader";

const char *const g_ImageRendererBillboardVertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec2 a_uv;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
  };

  void main()
  {
    gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);
    gl_Position.xy += u_screenSize.z * gl_Position.w * u_screenSize.xy * vec2(a_uv.x * 2.0 - 1.0, a_uv.y * 2.0 - 1.0); // expand billboard

    v_uv = vec2(a_uv.x, 1.0 - a_uv.y);
    v_colour = u_colour;
  }
)shader";

const char *const g_FlatColour_FragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec4 v_colour;

  //Output Format
  out vec4 out_Colour;

  void main()
  {
    out_Colour = v_colour;
  }
)shader";

const char *const g_DepthOnly_FragmentShader = FRAG_HEADER R"shader(
  //Output Format
  out vec4 out_Colour;

  void main()
  {
    out_Colour = vec4(0.0);
  }
)shader";

const char *const g_BlurVertexShader = VERT_HEADER R"shader(
  //Input format
  layout(location = 0) in vec3 a_position;
  layout(location = 1) in vec2 a_texCoord;

  //Output Format
  out vec2 v_uv0;
  out vec2 v_uv1;
  out vec2 v_uv2;

  layout (std140) uniform u_EveryFrame
  {
    vec4 u_stepSize; // remember: requires 16 byte alignment
  };

  void main()
  {
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);

    // sample on edges, taking advantage of bilinear sampling
    vec2 sampleOffset = 1.42 * u_stepSize.xy;
    vec2 uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
    v_uv0 = uv - sampleOffset;
    v_uv1 = uv;
    v_uv2 = uv + sampleOffset;
  }
)shader";

const char *const g_BlurFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv0;
  in vec2 v_uv1;
  in vec2 v_uv2;

  //Output Format
  out vec4 out_Colour;

  uniform sampler2D u_texture;

  vec4 kernel[3] = vec4[](vec4(0.0, 0.0, 0.0, 0.27901),
                          vec4(1.0, 1.0, 1.0, 0.44198),
                          vec4(0.0, 0.0, 0.0, 0.27901));

  void main()
  {
    vec4 colour = vec4(0);

    colour += kernel[0] * texture(u_texture, v_uv0);
    colour += kernel[1] * texture(u_texture, v_uv1);
    colour += kernel[2] * texture(u_texture, v_uv2);

    out_Colour = colour;
  }

)shader";

const char *const g_HighlightVertexShader = VERT_HEADER R"shader(
  //Input format
  layout(location = 0) in vec3 a_position;
  layout(location = 1) in vec2 a_texCoord;

  //Output Format
  out vec2 v_uv0;
  out vec2 v_uv1;
  out vec2 v_uv2;
  out vec2 v_uv3;
  out vec2 v_uv4;

  vec2 searchKernel[4] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1,  1), vec2(1,  1));

  layout (std140) uniform u_EveryFrame
  {
    vec4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
    vec4 u_colour;
  };

  void main()
  {
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);

    v_uv0 = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
    v_uv1 = v_uv0 + u_stepSizeThickness.xy * searchKernel[0];
    v_uv2 = v_uv0 + u_stepSizeThickness.xy * searchKernel[1];
    v_uv3 = v_uv0 + u_stepSizeThickness.xy * searchKernel[2];
    v_uv4 = v_uv0 + u_stepSizeThickness.xy * searchKernel[3];
  }
)shader";

const char *const g_HighlightFragmentShader = FRAG_HEADER R"shader(
  //Input Format
  in vec2 v_uv0;
  in vec2 v_uv1;
  in vec2 v_uv2;
  in vec2 v_uv3;
  in vec2 v_uv4;

  //Output Format
  out vec4 out_Colour;

  uniform sampler2D u_texture;
  layout (std140) uniform u_EveryFrame
  {
    vec4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
    vec4 u_colour;
  };

  void main()
  {
    vec4 middle = texture(u_texture, v_uv0);
    float result = middle.w;

    // 'outside' the geometry, just use the blurred 'distance'
    if (middle.x == 0.0)
    {
      out_Colour = vec4(u_colour.xyz, result * u_stepSizeThickness.z * u_colour.a);
      return;
    }

    result = 1.0 - result;

    // look for an edge, setting to full colour if found
    float softenEdge = 0.15 * u_colour.a;
    result += softenEdge * step(texture(u_texture, v_uv1).x - middle.x, -0.00001);
    result += softenEdge * step(texture(u_texture, v_uv2).x - middle.x, -0.00001);
    result += softenEdge * step(texture(u_texture, v_uv3).x - middle.x, -0.00001);
    result += softenEdge * step(texture(u_texture, v_uv4).x - middle.x, -0.00001);

    result = max(u_stepSizeThickness.w, result) * u_colour.w; // overlay colour
    out_Colour = vec4(u_colour.xyz, result);
  }

)shader";


const char *const g_udGPURenderQuadVertexShader = VERT_HEADER R"shader(
  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec2 a_corner;

  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
  };

  void main()
  {
    v_colour = a_color.bgra;

    // Points
    vec4 off = vec4(a_position.www * 2.0, 0);
    vec4 pos0 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.www, 1.0);
    vec4 pos1 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xww, 1.0);
    vec4 pos2 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyw, 1.0);
    vec4 pos3 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyw, 1.0);
    vec4 pos4 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wwz, 1.0);
    vec4 pos5 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xwz, 1.0);
    vec4 pos6 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyz, 1.0);
    vec4 pos7 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyz, 1.0);

    vec4 minPos, maxPos;
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
    gl_Position = (minPos + (maxPos - minPos) * 0.5);

    vec2 pointSize = vec2(maxPos.x - minPos.x, maxPos.y - minPos.y);
    gl_Position.xy += pointSize * a_corner * 0.5;
  }
)shader";

const char *const g_udGPURenderQuadFragmentShader = FRAG_HEADER R"shader(
  in vec4 v_colour;

  out vec4 out_Colour;

  void main()
  {
    out_Colour = v_colour;
  }
)shader";


const char *const g_udGPURenderGeomVertexShader = VERT_HEADER R"shader(
  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_colour;

  out vec4 v_colour;
  out vec2 v_pointSize;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
  };

  void main()
  {
    v_colour = vec4(a_colour.bgr * u_colour.xyz, u_colour.w);

    // Points
    vec4 off = vec4(a_position.www * 2.0, 0);
    vec4 pos0 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.www, 1.0);
    vec4 pos1 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xww, 1.0);
    vec4 pos2 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyw, 1.0);
    vec4 pos3 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyw, 1.0);
    vec4 pos4 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wwz, 1.0);
    vec4 pos5 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xwz, 1.0);
    vec4 pos6 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyz, 1.0);
    vec4 pos7 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyz, 1.0);

    vec4 minPos, maxPos;
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
    gl_Position = (minPos + (maxPos - minPos) * 0.5);

    v_pointSize = vec2(maxPos.x - minPos.x, maxPos.y - minPos.y);
  }
)shader";

const char *const g_udGPURenderGeomFragmentShader = FRAG_HEADER R"shader(
  in vec4 g_colour;

  out vec4 out_Colour;

  void main()
  {
    out_Colour = g_colour;
  }
)shader";

const char *const g_udGPURenderGeomGeometryShader = FRAG_HEADER R"shader(
  layout(points) in;
  layout(triangle_strip, max_vertices=4) out;

  in vec2 v_pointSize[];
  in vec4 v_colour[];
  out vec4 g_colour;

  void main()
  {
    g_colour = v_colour[0];

    vec2 halfPointSize = v_pointSize[0] * vec2(0.5);

    gl_Position = gl_in[0].gl_Position + vec4(-halfPointSize.x, -halfPointSize.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(-halfPointSize.x, halfPointSize.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(halfPointSize.x, -halfPointSize.y, 0.0, 0.0);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(halfPointSize.x, halfPointSize.y, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
  }
)shader";

const char *const g_PostEffectsVertexShader = FRAG_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;
out vec2 v_edgeSampleUV0;
out vec2 v_edgeSampleUV1;
out vec2 v_edgeSampleUV2;
out vec2 v_edgeSampleUV3;
out vec2 v_sampleStepSize;
out float v_saturation;

layout (std140) uniform u_params
{
  vec4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, (unused), (unused)
  vec4 u_saturation; // saturation, (unused), (unused), (unused)
};

void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
  v_sampleStepSize = u_screenParams.xy;

  // sample corners
  v_edgeSampleUV0 = v_uv + u_screenParams.xy;
  v_edgeSampleUV1 = v_uv - u_screenParams.xy;
  v_edgeSampleUV2 = v_uv + vec2(u_screenParams.x, -u_screenParams.y);
  v_edgeSampleUV3 = v_uv + vec2(-u_screenParams.x, u_screenParams.y);

  v_saturation = u_saturation.x;
}
)shader";

const char *const g_PostEffectsFragmentShader = FRAG_HEADER R"shader(

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
#define FxaaDiscard discard
#define FxaaFloat float
#define FxaaFloat2 vec2
#define FxaaFloat3 vec3
#define FxaaFloat4 vec4
#define FxaaHalf float
#define FxaaHalf2 vec2
#define FxaaHalf3 vec3
#define FxaaHalf4 vec4
#define FxaaInt2 vec2
#define FxaaSat(x) clamp(x, 0.0, 1.0)
#define FxaaTex sampler2D

#define FxaaTexTop(t, p) textureLod(t, p, 0.0)
#define FxaaTexOff(t, p, o, r) textureLod(t, p + (o * r), 0.0)

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

vec3 saturation(vec3 rgb, float adjustment)
{
  const vec3 W = vec3(0.2125, 0.7154, 0.0721);
  float intensity = dot(rgb, W);
  return mix(vec3(intensity), rgb, adjustment);
}

uniform sampler2D u_texture;
uniform sampler2D u_depth;
  
//Input Format
in vec2 v_uv;
in vec2 v_edgeSampleUV0;
in vec2 v_edgeSampleUV1;
in vec2 v_edgeSampleUV2;
in vec2 v_edgeSampleUV3;
in vec2 v_sampleStepSize;
in float v_saturation;

//Output Format
out vec4 out_Colour;

void main()
{
  vec4 colour = vec4(0.0, 0.0, 0.0, 0.0);
  float depth = texture(u_depth, v_uv).x;

  // only run FXAA on edges (simple edge detection)
  float depth0 = texture(u_depth, v_edgeSampleUV0).x;
  float depth1 = texture(u_depth, v_edgeSampleUV1).x;
  float depth2 = texture(u_depth, v_edgeSampleUV2).x;
  float depth3 = texture(u_depth, v_edgeSampleUV3).x;
	
  const float edgeThreshold = 0.003;
  float isEdge = 1.0 - (step(abs(depth0 - depth), edgeThreshold) * step(abs(depth1 - depth), edgeThreshold) * step(abs(depth2 - depth), edgeThreshold) * step(abs(depth3 - depth), edgeThreshold));
  if (isEdge == 0.0)
  {
    colour = texture(u_texture, v_uv);
  }
  else
  {
    colour = FxaaPixelShader(v_uv, vec4(0.0, 0.0, 0.0, 0.0), u_texture, u_texture, u_texture, v_sampleStepSize,
                             vec4(0.0, 0.0, 0.0, 0.0), vec4(0.0, 0.0, 0.0, 0.0), vec4(0.0, 0.0, 0.0, 0.0),
                             0.75,  //fxaaQualitySubpix
                             0.125, // fxaaQualityEdgeThreshold
                             0.0, // fxaaQualityEdgeThresholdMin
                             0.0, 0.0, vec4(0.0, 0.0, 0.0, 0.0));
  }
   
  out_Colour = vec4(saturation(colour.xyz, v_saturation), 1.0);
}
)shader";
