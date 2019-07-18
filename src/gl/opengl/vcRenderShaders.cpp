#include "gl/vcRenderShaders.h"
#include "udPlatformUtil.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN
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
  if (edgeOutlineWidth > 0.0 && u_outlineColour.w > 0.0)
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

const char* const g_tileFragmentShader = FRAG_HEADER R"shader(
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

const char* const g_tileVertexShader = VERT_HEADER R"shader(
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


const char* const g_vcSkyboxVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_texCoord;

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_texCoord = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
}
)shader";

const char* const g_vcSkyboxFragmentShader = FRAG_HEADER R"shader(
uniform sampler2D u_texture;
layout (std140) uniform u_EveryFrame
{
  mat4 u_inverseViewProjection;
};

//Input Format
in vec2 v_texCoord;

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
  vec4 point3D = u_inverseViewProjection * vec4(v_texCoord * vec2(2.0) - vec2(1.0), 1.0, 1.0);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  vec4 c1 = texture(u_texture, directionToLatLong(point3D.xyz));

  out_Colour = c1;
}
)shader";


const char* const g_CompassFragmentShader = FRAG_HEADER R"shader(
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

const char* const g_CompassVertexShader = VERT_HEADER R"shader(
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

const char* const g_FenceVertexShader = VERT_HEADER R"shader(
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
  mat4 u_modelViewProjectionMatrix;
};

void main()
{
  // fence horizontal UV pos packed into Y channel
  v_uv = vec2(mix(a_uv.y, a_uv.x, u_orientation) * u_textureRepeatScale - u_time * u_textureScrollSpeed, a_ribbonInfo.w);
  v_colour = mix(u_bottomColour, u_topColour, a_ribbonInfo.w);

  // fence or flat
  vec3 worldPosition = a_position + mix(vec3(0, 0, a_ribbonInfo.w) * u_width, a_ribbonInfo.xyz, u_orientation);

  gl_Position = u_modelViewProjectionMatrix * vec4(worldPosition, 1.0);
}
)shader";

const char* const g_FenceFragmentShader = FRAG_HEADER R"shader(
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

const char* const g_WaterFragmentShader = FRAG_HEADER R"shader(
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

const char* const g_WaterVertexShader = VERT_HEADER R"shader(
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
    mat4 u_modelViewMatrix;
    mat4 u_modelViewProjectionMatrix;
  };

  void main()
  {
    float uvScaleBodySize = u_colourAndSize.w; // packed here

    // scale the uvs with time
    float uvOffset = u_time.x * 0.0625;
    v_uv0 = uvScaleBodySize * a_position.xy * vec2(0.25) - vec2(uvOffset, uvOffset);
    v_uv1 = uvScaleBodySize * a_position.yx * vec2(0.50) - vec2(uvOffset, uvOffset * 0.75);

    v_fragEyePos = u_modelViewMatrix * vec4(a_position, 0.0, 1.0);
    v_colour = u_colourAndSize.xyz;

    gl_Position = u_modelViewProjectionMatrix * vec4(a_position, 0.0, 1.0);
  }
)shader";

const char* const g_PolygonP1N1UV1FragmentShader = FRAG_HEADER R"shader(
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
    out_Colour = col * v_colour;
  }
)shader";

const char* const g_PolygonP1N1UV1VertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec3 a_normal;
  layout(location = 2) in vec2 a_uv;
  //layout(location = 3) in vec4 a_colour;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;
  out vec3 v_normal;

  layout (std140) uniform u_EveryFrame
  {
    mat4 u_viewProjectionMatrix;
  };

  layout (std140) uniform u_EveryObject
  {
    mat4 u_modelMatrix;
    vec4 u_colour;
  };

  void main()
  {
    gl_Position = u_viewProjectionMatrix * u_modelMatrix * vec4(a_pos, 1.0);

    v_uv = a_uv;
    v_normal = a_normal;
    v_colour = u_colour;// * a_colour;
  }
)shader";

const char* const g_PolygonP1UV1FragmentShader = FRAG_HEADER R"shader(
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

const char* const g_PolygonP1UV1VertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec2 a_uv;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_modelViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize; // unused
  };

  void main()
  {
    gl_Position = u_modelViewProjectionMatrix * vec4(a_pos, 1.0);

    v_uv = vec2(a_uv.x, 1.0 - a_uv.y);
    v_colour = u_colour;
  }
)shader";

const char* const g_BillboardFragmentShader = FRAG_HEADER R"shader(
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

const char* const g_BillboardVertexShader = VERT_HEADER R"shader(
  //Input Format
  layout(location = 0) in vec3 a_pos;
  layout(location = 1) in vec2 a_uv;

  //Output Format
  out vec2 v_uv;
  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_modelViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
  };

  void main()
  {
    gl_Position = u_modelViewProjectionMatrix * vec4(a_pos, 1.0);
    gl_Position.xy += u_screenSize.z * gl_Position.w * u_screenSize.xy * vec2(a_uv.x * 2.0 - 1.0, a_uv.y * 2.0 - 1.0); // expand billboard

    v_uv = vec2(a_uv.x, 1.0 - a_uv.y);
    v_colour = u_colour;
  }
)shader";


const char *const g_udGPURenderQuadVertexShader = VERT_HEADER R"shader(
  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec2 a_corner;

  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProj;
  };

  void main()
  {
    v_colour = a_color.bgra;

    // Points
    vec4 off = vec4(a_position.www * 2.0, 0);
    vec4 pos0 = u_worldViewProj * vec4(a_position.xyz + off.www, 1.0);
    vec4 pos1 = u_worldViewProj * vec4(a_position.xyz + off.xww, 1.0);
    vec4 pos2 = u_worldViewProj * vec4(a_position.xyz + off.xyw, 1.0);
    vec4 pos3 = u_worldViewProj * vec4(a_position.xyz + off.wyw, 1.0);
    vec4 pos4 = u_worldViewProj * vec4(a_position.xyz + off.wwz, 1.0);
    vec4 pos5 = u_worldViewProj * vec4(a_position.xyz + off.xwz, 1.0);
    vec4 pos6 = u_worldViewProj * vec4(a_position.xyz + off.xyz, 1.0);
    vec4 pos7 = u_worldViewProj * vec4(a_position.xyz + off.wyz, 1.0);

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
  layout(location = 1) in vec4 a_color;

  out vec4 v_colour;
  out vec2 v_pointSize;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProj;
  };

  void main()
  {
    v_colour = a_color.bgra;

    // Points
    vec4 off = vec4(a_position.www * 2.0, 0);
    vec4 pos0 = u_worldViewProj * vec4(a_position.xyz + off.www, 1.0);
    vec4 pos1 = u_worldViewProj * vec4(a_position.xyz + off.xww, 1.0);
    vec4 pos2 = u_worldViewProj * vec4(a_position.xyz + off.xyw, 1.0);
    vec4 pos3 = u_worldViewProj * vec4(a_position.xyz + off.wyw, 1.0);
    vec4 pos4 = u_worldViewProj * vec4(a_position.xyz + off.wwz, 1.0);
    vec4 pos5 = u_worldViewProj * vec4(a_position.xyz + off.xwz, 1.0);
    vec4 pos6 = u_worldViewProj * vec4(a_position.xyz + off.xyz, 1.0);
    vec4 pos7 = u_worldViewProj * vec4(a_position.xyz + off.wyz, 1.0);

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