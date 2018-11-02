#include "gl/vcRenderShaders.h"
#include "udPlatform/udPlatformUtil.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# define FRAG_HEADER "#version 300 es\nprecision highp float;\n"
# define VERT_HEADER "#version 300 es\n"
#else
# define FRAG_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\n"
# define VERT_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\n"
#endif

const char* const g_udFragmentShader = FRAG_HEADER R"shader(
uniform sampler2D u_texture;
uniform sampler2D u_depth;

layout (std140) uniform u_params
{
  vec4 u_screenParams;  // sampleStepX, sampleStepSizeY, near plane, far plane
  mat4 u_inverseViewProjection;

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
  vec4 u_contourParams; // contour distance, contour band height, (unused), (unused)
};

//Input Format
in vec2 v_texCoord;

//Output Format
out vec4 out_Colour;

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

// depth is packed into .w component
vec4 edgeHighlight(vec3 col, vec2 uv, float depth)
{
  vec3 sampleOffsets = vec3(u_screenParams.xy, 0.0);
  float edgeOutlineThreshold = u_outlineParams.y;
  float farPlane = u_screenParams.w;

  float d1 = texture(u_depth, uv + sampleOffsets.xz).x;
  float d2 = texture(u_depth, uv - sampleOffsets.xz).x;
  float d3 = texture(u_depth, uv + sampleOffsets.zy).x;
  float d4 = texture(u_depth, uv - sampleOffsets.zy).x;

  float wd0 = linearizeDepth(depth) * farPlane;
  float wd1 = linearizeDepth(d1) * farPlane;
  float wd2 = linearizeDepth(d2) * farPlane;
  float wd3 = linearizeDepth(d3) * farPlane;
  float wd4 = linearizeDepth(d4) * farPlane;

  float isEdge = 1.0 - step(wd0 - wd1, edgeOutlineThreshold) * step(wd0 - wd2, edgeOutlineThreshold) * step(wd0 - wd3, edgeOutlineThreshold) * step(wd0 - wd4, edgeOutlineThreshold);

  vec3 edgeColour = mix(col.xyz, u_outlineColour.xyz, u_outlineColour.w);
  float minDepth = min(min(min(d1, d2), d3), d4);
  return vec4(mix(col.xyz, edgeColour, isEdge), mix(depth, minDepth, isEdge));
}

vec3 contourColour(vec3 col, vec3 fragWorldPosition)
{
  float contourDistance = u_contourParams.x;
  float contourBandHeight = u_contourParams.y;

  float isCountour = step(contourBandHeight, mod(fragWorldPosition.z, contourDistance));
  vec3 contourColour = mix(col.xyz, u_contourColour.xyz, u_contourColour.w);
  return mix(contourColour, col.xyz, isCountour);
}

vec3 colourizeByHeight(vec3 col, vec3 fragWorldPosition)
{
  vec2 worldColourMinMax = u_colourizeHeightParams.xy;

  float minMaxColourStrength = getNormalizedPosition(fragWorldPosition.z, worldColourMinMax.x, worldColourMinMax.y);

  vec3 minColour = mix(col.xyz, u_colourizeHeightColourMin.xyz, u_colourizeHeightColourMin.w);
  vec3 maxColour = mix( col.xyz, u_colourizeHeightColourMax.xyz,u_colourizeHeightColourMax.w);
  return mix(minColour, maxColour, minMaxColourStrength);
}

vec3 colourizeByDepth(vec3 col, float depth)
{
  float farPlane = u_screenParams.w;
  float linearDepth = linearizeDepth(depth) * farPlane;
  vec2 depthColourMinMax = u_colourizeDepthParams.xy;

  float depthColourStrength = getNormalizedPosition(linearDepth, depthColourMinMax.x, depthColourMinMax.y);
  return mix(col.xyz, u_colourizeDepthColour.xyz, depthColourStrength * u_colourizeDepthColour.w);
}

void main()
{
  vec4 col = texture(u_texture, v_texCoord);
  float depth = texture(u_depth, v_texCoord).x;

  vec4 fragWorldPosition = u_inverseViewProjection * vec4(vec2(v_texCoord.x, 1.0 - v_texCoord.y) * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0, 1.0);
  fragWorldPosition /= fragWorldPosition.w;

  col.xyz = colourizeByHeight(col.xyz, fragWorldPosition.xyz);
  col.xyz = colourizeByDepth(col.xyz, depth);

  float edgeOutlineWidth = u_outlineParams.x;
  if (edgeOutlineWidth > 0.0 && u_outlineColour.w > 0)
  {
    vec4 edgeResult = edgeHighlight(col.xyz, v_texCoord, depth);
    col.xyz = edgeResult.xyz;
    depth = edgeResult.w; // to preserve outsides edges, depth written may be adjusted
  }
  col.xyz = contourColour(col.xyz, fragWorldPosition.xyz);

  out_Colour = vec4(col.xyz, 1.0); // UD always opaque
  gl_FragDepth = depth;
}
)shader";

const char* const g_udVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_texCoord;

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_texCoord = a_texCoord;
}
)shader";

const char* const g_terrainTileFragmentShader = FRAG_HEADER R"shader(
//Input Format
in vec4 v_colour;
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  out_Colour = vec4(col.xyz * v_colour.xyz, 1.0) * v_colour.w;
}
)shader";

const char* const g_terrainTileVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_uv;

//Output Format
out vec4 v_colour;
out vec2 v_uv;

// This should match CPU struct size
#define VERTEX_COUNT 3

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

const char* const g_vcSkyboxFragmentShader = FRAG_HEADER R"shader(

uniform samplerCube u_texture;
layout (std140) uniform u_EveryFrame
{
  mat4 u_inverseViewProjection;
};

//Input Format
in vec2 v_texCoord;

//Output Format
out vec4 out_Colour;

void main()
{
  vec2 uv = vec2(v_texCoord.x, 1.0 - v_texCoord.y);

  // work out 3D point
  vec4 point3D = u_inverseViewProjection * vec4(uv * vec2(2.0) - vec2(1.0), 1.0, 1.0);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  vec4 c1 = texture(u_texture, point3D.xyz);

  out_Colour = c1;
}
)shader";


const char* const g_PositionNormalFragmentShader = FRAG_HEADER R"shader(
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
    float edotr = max(0.0, dot(fakeEyeVector, worldNormal));
    edotr = pow(edotr, 60.0);
    vec3 sheenColour = vec3(1.0, 1.0, 0.9);
    out_Colour = vec4(v_colour.a * (ndotl * v_colour.xyz + edotr * sheenColour), 1.0);
  }
)shader";

const char* const g_PositionNormalVertexShader = VERT_HEADER R"shader(
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
    mat4 u_viewProjectionMatrix;
    vec4 u_colour;
    vec3 u_sunDirection;
    float _padding;
  };

  void main()
  {
    gl_Position = u_viewProjectionMatrix * vec4(a_pos, 1.0);

    v_normal = ((a_normal * 0.5) + 0.5);
    v_colour = u_colour;
    v_sunDirection = u_sunDirection;
    v_fragClipPosition = gl_Position;
  }
)shader";

const char* const g_ImGuiVertexShader = VERT_HEADER R"shader(
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

const char* const g_ImGuiFragmentShader = FRAG_HEADER R"shader(

uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main()
{
  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
}
)shader";
