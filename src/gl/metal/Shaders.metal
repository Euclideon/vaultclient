#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

// This should match CPU struct size
#define VERTEX_COUNT 2


// Visualization Vertex Shader - g_VisualizationVertexShader
  struct VVSInput
  {
    float3 a_position [[attribute(0)]];
    float2 a_texCoord [[attribute(1)]];
  };

  struct VVSOutput
  {
    float4 v_position [[position]];
    float2 uv;
  };

  vertex VVSOutput
  visualizationVertexShader(VVSInput in [[stage_in]])
  {
    VVSOutput out;
    out.v_position = float4(in.a_position.xy, 0.0, 1.0);
    out.uv = in.a_texCoord;
    return out;
  }

// Visualization Fragment Shader - g_VisualizationFragmentShader
struct VFSUniforms
{
  float4 u_screenParams;
  float4x4 u_inverseViewProjection;
  float4 u_outlineColor;
  float4 u_outlineParams;
  float4 u_colorizeHeightColorMin;
  float4 u_colorizeHeightColorMax;
  float4 u_colorizeHeightParams;
  float4 u_colorizeDepthColor;
  float4 u_colorizeDepthParams;
  float4 u_contourColor;
  float4 u_contourParams;
};

struct VFSOutput
{
  float4 out_Color [[color(0)]];
  float depth [[depth(any)]];
};

float3 hsv2rgb(float3 c)
{
  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

fragment VFSOutput
visualizationFragmentShader(VVSOutput in [[stage_in]],
                 constant VFSUniforms& uVFS [[buffer(1)]],
                 texture2d<float, access::sample> VFStexture [[texture(0)]],
                 sampler VFSsampler [[sampler(0)]],
                 depth2d<float, access::sample> VFSdepthTexture [[texture(1)]],
                 sampler VFSdepthSampler [[sampler(1)]])
{
  VFSOutput out;
  
  float4 col = VFStexture.sample(VFSsampler, in.uv);
  float depth = VFSdepthTexture.sample(VFSdepthSampler, in.uv);
  
  float farPlane = uVFS.u_screenParams.w;
  float nearPlane = uVFS.u_screenParams.z;
  
  float4 fragWorldPosition = uVFS.u_inverseViewProjection * float4(in.uv.x * 2.0 - 1.0, (1.0 - in.uv.y) * 2.0 - 1.0, depth, 1.0);
  fragWorldPosition = fragWorldPosition / fragWorldPosition.w;
  
  float2 worldColorMinMax = uVFS.u_colorizeHeightParams.xy;
  float minMaxColorStrength = clamp((fragWorldPosition.z - worldColorMinMax.x) / (worldColorMinMax.y - worldColorMinMax.x), 0.0, 1.0);
  float3 minColor = mix(col.xyz, uVFS.u_colorizeHeightColorMin.xyz, uVFS.u_colorizeHeightColorMin.w);
  float3 maxColor = mix(col.xyz, uVFS.u_colorizeHeightColorMax.xyz, uVFS.u_colorizeHeightColorMax.w);
  col.xyz = mix(minColor, maxColor, minMaxColorStrength);
  
  float linearDepth = ((2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane))) * farPlane;
  float2 depthColorMinMax = uVFS.u_colorizeDepthParams.xy;

  float depthColorStrength = clamp((linearDepth - depthColorMinMax.x) / (depthColorMinMax.y - depthColorMinMax.x), 0.0, 1.0);
  
  col.xyz = mix(col.xyz, uVFS.u_colorizeDepthColor.xyz, depthColorStrength * uVFS.u_colorizeDepthColor.w);

  float contourBandHeight = uVFS.u_contourParams.y;
  float contourRainboxRepeat = uVFS.u_contourParams.z;
  float contourRainboxIntensity = uVFS.u_contourParams.w;

  float3 rainbowColor = hsv2rgb(float3(fragWorldPosition.z * (1.0 / contourRainboxRepeat), 1.0, 1.0));
  float3 baseColor = mix(col.xyz, rainbowColor, contourRainboxIntensity);

  float isContour = 1.0 - step(contourBandHeight, fmod(abs(fragWorldPosition.z), uVFS.u_contourParams.x));

  col.xyz = mix(baseColor, uVFS.u_contourColor.xyz, isContour * uVFS.u_contourColor.w);

  float edgeOutlineWidth = uVFS.u_outlineParams.x;
  if (edgeOutlineWidth > 0.0 && uVFS.u_outlineColor.w > 0.0)
  {
    float3 sampleOffsets = float3(uVFS.u_screenParams.xy, 0.0);
    float edgeOutlineThreshold = uVFS.u_outlineParams.y;
    
    float d1 = VFSdepthTexture.sample(VFSdepthSampler, in.uv + sampleOffsets.xz);
    float d2 = VFSdepthTexture.sample(VFSdepthSampler, in.uv - sampleOffsets.xz);
    float d3 = VFSdepthTexture.sample(VFSdepthSampler, in.uv + sampleOffsets.zy);
    float d4 = VFSdepthTexture.sample(VFSdepthSampler, in.uv - sampleOffsets.zy);
    
    float wd0 = ((2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane))) * farPlane;
    float wd1 = ((2.0 * nearPlane) / (farPlane + nearPlane - d1 * (farPlane - nearPlane))) * farPlane;
    float wd2 = ((2.0 * nearPlane) / (farPlane + nearPlane - d2 * (farPlane - nearPlane))) * farPlane;
    float wd3 = ((2.0 * nearPlane) / (farPlane + nearPlane - d3 * (farPlane - nearPlane))) * farPlane;
    float wd4 = ((2.0 * nearPlane) / (farPlane + nearPlane - d4 * (farPlane - nearPlane))) * farPlane;
    
    float isEdge = 1.0 - step(wd0 - wd1, edgeOutlineThreshold) * step(wd0 - wd2, edgeOutlineThreshold) * step(wd0 - wd3, edgeOutlineThreshold) * step(wd0 - wd4, edgeOutlineThreshold);
    
    float3 edgeColor = mix(col.xyz, uVFS.u_outlineColor.xyz, uVFS.u_outlineColor.w);
    float minDepth = min(min(min(d1, d2), d3), d4);
    float4 edgeResult = float4(mix(col.xyz, edgeColor, isEdge), (depth + isEdge * (minDepth - depth)));
    
    col.xyz = edgeResult.xyz;
    depth = edgeResult.w; // to preserve outsides edges, depth written may be adjusted
  }

  out.out_Color = float4(col.rgb, 1.0);
  out.depth = depth;
  
  return out;
}

// Compass Vertex Shader - g_CompassVertexShader
  struct CVSInput
  {
    float3 a_pos [[attribute(0)]];
    float3 a_normal [[attribute(1)]];
  };

  struct CVSOutput
  {
    float4 pos [[position]];
    float4 v_color;
    float3 v_normal;
    float4 v_fragClipPosition;
    float3 v_sunDirection;
  };

  struct CVSUniforms
  {
    float4x4 u_worldViewProjectionMatrix;
    float4 u_color;
    float3 u_sunDirection;
  };

  vertex CVSOutput
  compassVertexShader(CVSInput in [[stage_in]], constant CVSUniforms& uCVS [[buffer(1)]])
  {
    CVSOutput out;
    out.v_fragClipPosition = uCVS.u_worldViewProjectionMatrix * float4(in.a_pos, 1.0);
    out.pos = out.v_fragClipPosition;
    out.v_normal = (in.a_normal * 0.5) + 0.5;
    out.v_color = uCVS.u_color;
    out.v_sunDirection = uCVS.u_sunDirection;
    return out;
  }

// Compass Fragment Shader - g_CompassFragmentShader
  fragment float4
  compassFragmentShader(CVSOutput in [[stage_in]])
  {
    float3 fakeEyeVector = normalize(in.v_fragClipPosition.xyz / in.v_fragClipPosition.w);
    float3 worldNormal = in.v_normal * float3(2.0) - float3(1.0);
    float ndotl = 0.5 + 0.5 * (-dot(in.v_sunDirection, worldNormal));
    float edotr = max(0.0, -dot(-fakeEyeVector, worldNormal));
    edotr = pow(edotr, 60.0);
    float3 sheenColor = float3(1.0, 1.0, 0.9);
    return float4(in.v_color.a * (ndotl * in.v_color.rgb + edotr * sheenColor), 1.0);
  }

// Tile Vertex Shader - g_tileVertexShader
  struct TVSInput
  {
    float3 a_uv [[attribute(0)]];
  };

  struct PCU
  {
    float4 v_position [[position]];
    float4 v_color;
    float2 v_uv;
  };

  struct TVSUniforms
  {
    float4x4 u_projection;
    float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
    float4 u_color;
  };

  vertex PCU
  tileVertexShader(TVSInput in [[stage_in]], constant TVSUniforms& uTVS [[buffer(1)]])
  {
    PCU out;
    
    // TODO: could have precision issues on some devices
    out.v_position = uTVS.u_projection * uTVS.u_eyePositions[int(in.a_uv.z)];
    
    out.v_uv = in.a_uv.xy;
    out.v_color = uTVS.u_color;
    return out;
  }

// Tile Fragment Shader - g_tileFragmentShader
  fragment float4
  tileFragmentShader(PCU in [[stage_in]], texture2d<float, access::sample> TFSimg [[texture(0)]], sampler TFSsampler [[sampler(0)]])
  {
    float4 col = TFSimg.sample(TFSsampler, in.v_uv);
    return float4(col.xyz * in.v_color.xyz, in.v_color.w);
  };

// Skybox Vertex Shader Panorama - g_vcSkyboxVertexShaderPanorama
  struct SVSInput
  {
    float3 a_position [[attribute(0)]];
    float2 a_texCoord [[attribute(1)]];
  };

  struct SVSOutput
  {
    float4 position [[position]];
    float2 v_texCoord;
  };

  vertex SVSOutput
  skyboxVertexShaderPanorama(SVSInput in [[stage_in]])
  {
    SVSOutput out;
    out.position = float4(in.a_position.xy, 0.0, 1.0);
    out.v_texCoord = float2(in.a_texCoord.x, 1.0 - in.a_texCoord.y);
    return out;
  }


// Skybox Fragment Shader Panorama - skyboxFragmentShaderPanorama
  struct SFSPUniforms
  {
    float4x4 u_inverseViewProjection;
  };

  fragment float4
  skyboxFragmentShaderPanorama(SVSOutput in [[stage_in]], constant SFSPUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
  {
    // work out 3D point
    float4 point3D = uSFS.u_inverseViewProjection * float4(in.v_texCoord * float2(2.0) - float2(1.0), 1.0, 1.0);
    point3D.xyz = normalize(point3D.xyz / point3D.w);
    float2 longlat = float2(atan2(point3D.x, point3D.y) + M_PI_F, acos(point3D.z));
    return SFSimg.sample(SFSsampler, longlat / float2(2.0 * M_PI_F, M_PI_F));
  }

// Skybox Fragment Shader TintColor - skyboxFragmentShaderImageColor
  struct SFSTCUniforms
  {
    float4 u_tintColor; //0 is full color, 1 is full image
    float4 u_imageSize; //For purposes of tiling/stretching
  };

  fragment float4
  skyboxFragmentShaderImageColor(SVSOutput in [[stage_in]], constant SFSTCUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
  {
    float4 color = SFSimg.sample(SFSsampler, in.v_texCoord / uSFS.u_imageSize.xy);
    float effectiveAlpha = min(color.a, uSFS.u_tintColor.a);
    return float4((color.rgb * effectiveAlpha) + (uSFS.u_tintColor.rgb * (1 - effectiveAlpha)), 1);
  }


// ImGui Vertex Shader - g_ImGuiVertexShader
  struct Uniforms {
      float4x4 projectionMatrix;
  };

  struct VertexIn {
      float2 position  [[attribute(0)]];
      float2 texCoords [[attribute(1)]];
      float4 color     [[attribute(2)]];
  };

  struct VertexOut {
    float4 position [[position]];
    float2 texCoords;
    float4 color;
  };

  vertex VertexOut imguiVertexShader(VertexIn in [[stage_in]], constant Uniforms& uIMGUI [[buffer(1)]])
  {
      VertexOut out;
      out.position = uIMGUI.projectionMatrix * float4(in.position, 0, 1);
      out.texCoords = in.texCoords;
      out.color = in.color;
      return out;
  }


// ImGui Fragment Shader - g_ImGuiFragmentShader
  fragment half4 imguiFragmentShader(VertexOut in [[stage_in]], texture2d<half, access::sample> IMtexture [[texture(0)]], sampler imguiSampler [[sampler(0)]])
  {
    half4 texColor = IMtexture.sample(imguiSampler, in.texCoords);
    return half4(in.color) * texColor;
  }

// Fence Vertex Shader - g_FenceVertexShader

  struct FVSUniforms
  {
    float4 u_bottomColor;
    float4 u_topColor;

    float u_orientation;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
  };

  struct FVSEveryObject
  {
    float4x4 u_worldViewProjectionMatrix;
  };

  struct FVSInput
  {
    float3 a_position [[attribute(0)]];
    float2 a_uv [[attribute(1)]];
    float4 a_ribbonInfo [[attribute(2)]]; // xyz: expand floattor; z: pair id (0 or 1)
  };

  vertex PCU
  fenceVertexShader(FVSInput in [[stage_in]], constant FVSUniforms& uFVS [[buffer(1)]], constant FVSEveryObject& uFVSEO [[buffer(2)]])
  {
    PCU out;

    // fence horizontal UV pos packed into Y channel
    out.v_uv = float2(mix(in.a_uv.y, in.a_uv.x, uFVS.u_orientation) * uFVS.u_textureRepeatScale - uFVS.u_time * uFVS.u_textureScrollSpeed, in.a_ribbonInfo.w);
    out.v_color = mix(uFVS.u_bottomColor, uFVS.u_topColor, in.a_ribbonInfo.w);

    // fence or flat
    float3 worldPosition = in.a_position + mix(float3(0, 0, in.a_ribbonInfo.w) * uFVS.u_width, in.a_ribbonInfo.xyz, uFVS.u_orientation);

    out.v_position = uFVSEO.u_worldViewProjectionMatrix * float4(worldPosition, 1.0);
    return out;
  }


// Fence Fragment Shader - g_FenceFragmentShader
  fragment float4 fenceFragmentShader(PCU in [[stage_in]], texture2d<float, access::sample> FFSimg [[texture(0)]], sampler FFSsampler [[sampler(0)]])
  {
    float4 texCol = FFSimg.sample(FFSsampler, in.v_uv);
    return float4(texCol.xyz * in.v_color.xyz, texCol.w * in.v_color.w);
  }

// UD Vertex Shader - g_udVertexShader
  struct UDVSInput
  {
    float3 a_position [[attribute(0)]];
    float2 a_texCoord [[attribute(1)]];
  };

  struct UDVSOutput
  {
    float4 v_position [[position]];
    float2 uv;
  };

  vertex UDVSOutput
  udVertexShader(UDVSInput in [[stage_in]])
  {
    UDVSOutput out;
    out.v_position = float4(in.a_position.xy, 0.0, 1.0);
    out.uv = in.a_texCoord;
    return out;
  }

// UD Fragment Shader - g_UDFragmentShader
struct UDFSUniforms
{
  float4 u_screenParams;
  float4x4 u_inverseViewProjection;
  float4 u_outlineColor;
  float4 u_outlineParams;
  float4 u_colorizeHeightColorMin;
  float4 u_colorizeHeightColorMax;
  float4 u_colorizeHeightParams;
  float4 u_colorizeDepthColor;
  float4 u_colorizeDepthParams;
  float4 u_contourColor;
  float4 u_contourParams;
};

struct UDFSOutput
{
  float4 out_Color [[color(0)]];
  float depth [[depth(any)]];
};

fragment UDFSOutput
udFragmentShader(UDVSOutput in [[stage_in]],
                 constant UDFSUniforms& uUDFS [[buffer(1)]],
                 texture2d<float, access::sample> UDFStexture [[texture(0)]],
                 sampler UDFSsampler [[sampler(0)]],
                 depth2d<float, access::sample> UDFSdepthTexture [[texture(1)]],
                 sampler UDFSdepthSampler [[sampler(1)]])
{
  UDFSOutput out;
  
  float4 col = UDFStexture.sample(UDFSsampler, in.uv);
  float depth = UDFSdepthTexture.sample(UDFSdepthSampler, in.uv);
  
  out.out_Color = float4(col.rgb, 1.0);// UD always opaque
  out.depth = depth;
  
  return out;
}

// g_WaterVertexShader
struct WVSInput
{
    float2 pos [[attribute(0)]];
};

struct WVSOutput
{
    float4 pos [[position]];
    float2 uv0;
    float2 uv1;
    float4 fragEyePos;
    float3 color;
};

struct WVSUniforms1
{
    float4 u_time;
};

struct WVSUniforms2
{
    float4 u_colorAndSize;
    float4x4 u_worldViewMatrix;
    float4x4 u_worldViewProjectionMatrix;
};

vertex WVSOutput
waterVertexShader(WVSInput in [[stage_in]], constant WVSUniforms1& uWVS1 [[buffer(1)]], constant WVSUniforms2& uWVS2 [[buffer(3)]])
{
    WVSOutput out;
    
    float uvScaleBodySize = uWVS2.u_colorAndSize.w; // packed here
    
    // scale the uvs with time
    float uvOffset = uWVS1.u_time.x * 0.0625;
    out.uv0 = uvScaleBodySize * in.pos.xy * float2(0.25, 0.25) - float2(uvOffset);
    out.uv1 = uvScaleBodySize * in.pos.yx * float2(0.50, 0.50) - float2(uvOffset, uvOffset * 0.75);
    
    out.fragEyePos = uWVS2.u_worldViewMatrix * float4(in.pos, 0.0, 1.0);
    out.color = uWVS2.u_colorAndSize.xyz;
    out.pos = uWVS2.u_worldViewProjectionMatrix * float4(in.pos, 0.0, 1.0);
    
    return out;
}

// g_WaterFragmentShader
struct WFSUniforms
{
  float4 u_specularDir;
  float4x4 u_eyeNormalMatrix;
  float4x4 u_inverseViewMatrix;
};

float2 directionToLatLong(float3 dir)
{
  float2 longlat = float2(atan2(dir.x, dir.y) + M_PI_F, acos(dir.z));
  return longlat / float2(2.0 * M_PI_F, M_PI_F);
}

fragment float4
waterFragmentShader(WVSOutput in [[stage_in]], constant WFSUniforms& uWFS [[buffer(2)]], texture2d<float, access::sample> WFSNormal [[texture(0)]], sampler WFSNsampler [[sampler(0)]], texture2d<float, access::sample> WFSSkybox [[texture(1)]], sampler WFSSBsampler [[sampler(1)]])
{
  float3 specularDir = normalize(uWFS.u_specularDir.xyz);
  
  float3 normal0 = WFSNormal.sample(WFSNsampler, in.uv0).xyz * float3(2.0) - float3(1.0);
  float3 normal1 = WFSNormal.sample(WFSNsampler, in.uv1).xyz * float3(2.0) - float3(1.0);
  float3 normal = normalize((normal0.xyz * normal1.xyz));
  
  float3 eyeToFrag = normalize(in.fragEyePos.xyz);
  float3 eyeSpecularDir = normalize((uWFS.u_eyeNormalMatrix * float4(specularDir, 0.0)).xyz);
  float3 eyeNormal = normalize((uWFS.u_eyeNormalMatrix * float4(normal, 0.0)).xyz);
  float3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));
  
  float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
  float nDotL = -dot(eyeNormal, eyeToFrag);
  float fresnel = nDotL * 0.5 + 0.5;
  
  float specular = pow(nDotS, 250.0) * 0.5;
  
  float3 deepFactor = float3(0.35, 0.35, 0.35);
  float3 shallowFactor = float3(1.0, 1.0, 0.7);
  
  float distanceToShore = 1.0; // maybe TODO
  float3 refractionColor = in.color.xyz * mix(shallowFactor, deepFactor, distanceToShore);
  
  // reflection
  float4 worldFragPos = uWFS.u_inverseViewMatrix * float4(eyeReflectionDir, 0.0);
  float4 skybox = WFSSkybox.sample(WFSSBsampler, directionToLatLong(normalize(worldFragPos.xyz)));
  float3 reflectionColor = skybox.xyz;
  
  float3 finalColor = mix(reflectionColor, refractionColor, fresnel * 0.75) + float3(specular);
  return float4(finalColor, 1.0);
}

// g_PolygonP3N3UV2VertexShader
struct PNUVSInput
{
  float3 pos [[attribute(0)]];
  float3 normal [[attribute(1)]];
  float2 uv [[attribute(2)]];
};

struct PNUVSOutput
{
  float4 pos [[position]];
  float2 uv;
  float3 normal;
  float4 color;
};

struct PNUVSUniforms1
{
  float4x4 u_worldViewProjectionMatrix;
  float4x4 u_worldMatrix;
  float4 u_color;
};

vertex PNUVSOutput
polygonP3N3UV2VertexShader(PNUVSInput in [[stage_in]], constant PNUVSUniforms1& PNUVS1 [[buffer(1)]])
{
  PNUVSOutput out;

  // making the assumption that the model matrix won't contain non-uniform scale
  float3 worldNormal = normalize((PNUVS1.u_worldMatrix * float4(in.normal, 0.0)).xyz);

  out.pos = PNUVS1.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
  out.uv = in.uv;
  out.normal = worldNormal;
  out.color = PNUVS1.u_color;
  
  return out;
}


// g_PolygonP3N3UV2FragmentShader

fragment float4
polygonP3N3UV2FragmentShader(PNUVSOutput in [[stage_in]], texture2d<float, access::sample> PNUFSimg [[texture(0)]], sampler PNUFSsampler [[sampler(0)]])
{
    float4 col = PNUFSimg.sample(PNUFSsampler, in.uv);
    float4 diffuseColour = col * in.color;

    // some fixed lighting
    float3 lightDirection = normalize(float3(0.85, 0.15, 0.5));
    float ndotl = dot(in.normal, lightDirection) * 0.5 + 0.5;
    float3 diffuse = diffuseColour.xyz * ndotl;

    return float4(diffuse, diffuseColour.a);
}


// g_ImageRendererFragmentShader
struct PUC
{
    float4 pos [[position]];
    float2 uv;
    float4 color;
};

fragment float4
imageRendererFragmentShader(PUC in [[stage_in]], texture2d<float, access::sample> IRFSimg [[texture(0)]], sampler IRFSsampler [[sampler(0)]])
{
    float4 col = IRFSimg.sample(IRFSsampler, in.uv);
    return col * in.color;
}

// g_ImageRendererMeshVertexShader
struct PNUVVSInput
{
    float3 pos [[attribute(0)]];
    float3 normal [[attribute(1)]]; // unused
    float2 uv [[attribute(2)]];
};

struct IRMVSUniforms
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_color;
    float4 u_screenSize; // unused
};

vertex PUC
imageRendererMeshVertexShader(PNUVVSInput in [[stage_in]], constant IRMVSUniforms& uniforms [[buffer(1)]])
{
    PUC out;
    
    out.pos = uniforms.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
    out.uv = float2(in.uv[0], 1.0 - in.uv[1]);
    out.color = uniforms.u_color;
    
    return out;
}

// g_ImageRendererBillboardVertexShader
struct IRBVSInput
{
    float3 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct IRBVSUniforms
{
    float4x4 u_worldViewProjectionMatrix;
    float4 u_color;
    float4 u_screenSize;
};

vertex PUC
imageRendererBillboardVertexShader(IRBVSInput in [[stage_in]], constant IRBVSUniforms& uniforms [[buffer(1)]])
{
    PUC out;
    
    out.pos = uniforms.u_worldViewProjectionMatrix * float4(in.pos, 1.0);
    out.pos.xy += uniforms.u_screenSize.z * out.pos.w * uniforms.u_screenSize.xy * float2(in.uv.x * 2.0 - 1.0, in.uv.y * 2.0 - 1.0); // expand billboard
    out.uv = float2(in.uv.x, 1.0 - in.uv.y);
    
    out.color = uniforms.u_color;
    
    return out;
}

// g_udSplatIdFragmentShader
struct UDSOutput
{
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

struct UDSUniforms
{
    float4 u_idOverride;
};

bool floatEquals(float a, float b)
{
    return abs(a - b) <= 0.0015f;
}

fragment UDSOutput
udSplatIdFragmentShader(UDVSOutput in [[stage_in]], constant UDSUniforms& uniforms [[buffer(1)]], texture2d<float, access::sample> UDSimg [[texture(0)]], sampler UDSSampler [[sampler(0)]], depth2d<float, access::sample> UDSimg2 [[texture(1)]], sampler UDSSampler2 [[sampler(1)]])
{
    UDSOutput out;
    float4 col = UDSimg.sample(UDSSampler, in.uv);
    out.depth = UDSimg2.sample(UDSSampler2, in.uv);
    
    out.color = float4(0.0, 0.0, 0.0, 0.0);
    if ((uniforms.u_idOverride.w == 0.0 || floatEquals(uniforms.u_idOverride.w, col.w)))
    {
        out.color = float4(col.w, 0, 0, 1.0);
    }
    
    return out;
}

// g_FlatColor_FragmentShader
struct FCFSInput
{
    float4 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float4 color [[attribute(3)]];
};

float4 flatColorFragmentShader(FCFSInput in [[stage_in]])
{
    return float4(in.color);
}

// g_DepthOnly_FragmentShader
struct DOFSInput
{
    float4 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float4 color [[attribute(3)]];
};

float4 depthOnlyFragmentShader(DOFSInput in [[stage_in]])
{
    return float4(0.0, 0.0, 0.0, 0.0);
}

// g_BlurVertexShader
struct BlVSInput
{
    float3 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct BlVSOutput
{
    float4 pos [[position]];
    float2 uv0;
    float2 uv1;
    float2 uv2;
};

struct BlVSUniforms
{
    float4 u_stepSize; // remember: requires 16 byte alignment
};

vertex BlVSOutput
blurVertexShader(BlVSInput in [[stage_in]], constant BlVSUniforms& uniforms [[buffer(1)]])
{
    BlVSOutput out;
    
    out.pos = float4(in.pos.x, in.pos.y, 0.0, 1.0);
    
    // sample on edges, taking advantage of bilinear sampling
    float2 sampleOffset = 1.42 * uniforms.u_stepSize.xy;
    float2 uv = float2(in.uv.x, 1.0 - in.uv.y);
    out.uv0 = uv - sampleOffset;
    out.uv1 = uv;
    out.uv2 = uv + sampleOffset;
    
    return out;
}

// g_BlurFragmentShader
constant static float4 multipliers[3] = {float4(0.0, 0.0, 0.0, 0.27901), float4(1.0, 1.0, 1.0, 0.44198), float4(0.0, 0.0, 0.0, 0.27901)};

fragment float4
blurFragmentShader(BlVSOutput in [[stage_in]], texture2d<float, access::sample> BlFSimg [[texture(0)]], sampler BlFSSampler [[sampler(0)]])
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    
    color += multipliers[0] * BlFSimg.sample(BlFSSampler, in.uv0);
    color += multipliers[1] * BlFSimg.sample(BlFSSampler, in.uv1);
    color += multipliers[2] * BlFSimg.sample(BlFSSampler, in.uv2);
    
    return color;
}

// g_HighlightVertexShader
struct HVSInput
{
    float3 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct HFSInput
{
    float4 pos [[position]];
    float2 uv0;
    float2 uv1;
    float2 uv2;
    float2 uv3;
    float2 uv4;
};

constant static float2 searchKernel[4] = {float2(-1, -1), float2(1, -1), float2(-1,  1), float2(1, 1)};

struct HVSUniforms
{
    float4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
    float4 u_color;
};

vertex HFSInput
highlightVertexShader(HVSInput in [[stage_in]], constant HVSUniforms& uniforms [[buffer(1)]])
{
    HFSInput out;
    
    out.pos = float4(in.pos.x, in.pos.y, 0.0, 1.0);
    
    out.uv0 = in.uv;
    out.uv1 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[0];
    out.uv2 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[1];
    out.uv3 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[2];
    out.uv4 = in.uv + uniforms.u_stepSizeThickness.xy * searchKernel[3];
    
    return out;
}

// g_HighlightFragmentShader
fragment float4
highlightFragmentShader(HFSInput in [[stage_in]], constant HVSUniforms& uniforms [[buffer(1)]], texture2d<float, access::sample> HFSimg [[texture(0)]], sampler HFSSampler [[sampler(0)]])
{
    float4 middle = HFSimg.sample(HFSSampler, in.uv0);
    float result = middle.w;
    
    // 'outside' the geometry, just use the blurred 'distance'
    if (middle.x == 0.0)
        return float4(uniforms.u_color.xyz, result * uniforms.u_stepSizeThickness.z * uniforms.u_color.a);
    
    result = 1.0 - result;
    
    // look for an edge, setting to full color if found
    float softenEdge = 0.15 * uniforms.u_color.a;
    result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv1).x - middle.x, -0.00001);
    result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv2).x - middle.x, -0.00001);
    result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv3).x - middle.x, -0.00001);
    result += softenEdge * step(HFSimg.sample(HFSSampler, in.uv4).x - middle.x, -0.00001);
    
    result = max(uniforms.u_stepSizeThickness.w, result) * uniforms.u_color.w; // overlay color
    return float4(uniforms.u_color.xyz, result);
}

// g_postEffectsVertexShader

struct PEVParams
{
  float4 u_screenParams;
  float4 u_saturation;
};

struct PEVOutput
{
  float2 out_var_TEXCOORD0 [[user(locn0)]];
  float2 out_var_TEXCOORD1 [[user(locn1)]];
  float2 out_var_TEXCOORD2 [[user(locn2)]];
  float2 out_var_TEXCOORD3 [[user(locn3)]];
  float2 out_var_TEXCOORD4 [[user(locn4)]];
  float2 out_var_TEXCOORD5 [[user(locn5)]];
  float out_var_TEXCOORD6 [[user(locn6)]];
  float4 gl_Position [[position]];
};

struct PEVInput
{
  float3 in_var_POSITION [[attribute(0)]];
  float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex PEVOutput postEffectsVertexShader(PEVInput in [[stage_in]], constant PEVParams& u_params [[buffer(0)]])
{
  PEVOutput out = {};
  out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
  out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
  out.out_var_TEXCOORD1 = in.in_var_TEXCOORD0 + u_params.u_screenParams.xy;
  out.out_var_TEXCOORD2 = in.in_var_TEXCOORD0 - u_params.u_screenParams.xy;
  out.out_var_TEXCOORD3 = in.in_var_TEXCOORD0 + float2(u_params.u_screenParams.x, -u_params.u_screenParams.y);
  out.out_var_TEXCOORD4 = in.in_var_TEXCOORD0 + float2(-u_params.u_screenParams.x, u_params.u_screenParams.y);
  out.out_var_TEXCOORD5 = u_params.u_screenParams.xy;
  out.out_var_TEXCOORD6 = u_params.u_saturation.x;
  return out;
}

// g_postEffectsFragmentShader

constant float _66 = {};
constant float2 _67 = {};
constant float2 _68 = {};

struct PEFOutput
{
  float4 out_var_SV_Target [[color(0)]];
};

struct PEFInput
{
  float2 in_var_TEXCOORD0 [[user(locn0)]];
  float2 in_var_TEXCOORD1 [[user(locn1)]];
  float2 in_var_TEXCOORD2 [[user(locn2)]];
  float2 in_var_TEXCOORD3 [[user(locn3)]];
  float2 in_var_TEXCOORD4 [[user(locn4)]];
  float2 in_var_TEXCOORD5 [[user(locn5)]];
  float in_var_TEXCOORD6 [[user(locn6)]];
};

fragment PEFOutput main0(PEFInput in [[stage_in]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], sampler sampler0 [[sampler(0)]], sampler sampler1 [[sampler(1)]])
{
  PEFOutput out = {};
  float4 _80 = texture1.sample(sampler1, in.in_var_TEXCOORD0);
  float _81 = _80.x;
  float4 _535;
  if ((1.0 - (((step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD1).x - _81), 0.0030000000260770320892333984375) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD2).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD3).x - _81), 0.0030000000260770320892333984375)) * step(abs(texture1.sample(sampler1, in.in_var_TEXCOORD4).x - _81), 0.0030000000260770320892333984375))) == 0.0)
  {
    _535 = texture0.sample(sampler0, in.in_var_TEXCOORD0);
  }
  else
  {
    float4 _534;
    switch (0u)
    {
      default:
      {
        float2 _123 = _67;
        _123.x = in.in_var_TEXCOORD0.x;
        float2 _125 = _123;
        _125.y = in.in_var_TEXCOORD0.y;
        float4 _127 = texture0.sample(sampler0, _125, level(0.0));
        float4 _129 = texture0.sample(sampler0, _125, level(0.0), int2(0, 1));
        float _130 = _129.y;
        float4 _132 = texture0.sample(sampler0, _125, level(0.0), int2(1, 0));
        float _133 = _132.y;
        float4 _135 = texture0.sample(sampler0, _125, level(0.0), int2(0, -1));
        float _136 = _135.y;
        float4 _138 = texture0.sample(sampler0, _125, level(0.0), int2(-1, 0));
        float _139 = _138.y;
        float _140 = _127.y;
        float _147 = fast::max(fast::max(_136, _139), fast::max(_133, fast::max(_130, _140)));
        float _150 = _147 - fast::min(fast::min(_136, _139), fast::min(_133, fast::min(_130, _140)));
        if (_150 < fast::max(0.0, _147 * 0.125))
        {
          _534 = _127;
          break;
        }
        float4 _156 = texture0.sample(sampler0, _125, level(0.0), int2(-1));
        float _157 = _156.y;
        float4 _159 = texture0.sample(sampler0, _125, level(0.0), int2(1));
        float _160 = _159.y;
        float4 _162 = texture0.sample(sampler0, _125, level(0.0), int2(1, -1));
        float _163 = _162.y;
        float4 _165 = texture0.sample(sampler0, _125, level(0.0), int2(-1, 1));
        float _166 = _165.y;
        float _167 = _136 + _130;
        float _168 = _139 + _133;
        float _171 = (-2.0) * _140;
        float _174 = _163 + _160;
        float _180 = _157 + _166;
        bool _200 = (abs(((-2.0) * _139) + _180) + ((abs(_171 + _167) * 2.0) + abs(((-2.0) * _133) + _174))) >= (abs(((-2.0) * _130) + (_166 + _160)) + ((abs(_171 + _168) * 2.0) + abs(((-2.0) * _136) + (_157 + _163))));
        bool _203 = !_200;
        float _204 = _203 ? _139 : _136;
        float _205 = _203 ? _133 : _130;
        float _209;
        if (_200)
        {
          _209 = in.in_var_TEXCOORD5.y;
        }
        else
        {
          _209 = in.in_var_TEXCOORD5.x;
        }
        float _216 = abs(_204 - _140);
        float _217 = abs(_205 - _140);
        bool _218 = _216 >= _217;
        float _223;
        if (_218)
        {
          _223 = -_209;
        }
        else
        {
          _223 = _209;
        }
        float _226 = fast::clamp(abs(((((_167 + _168) * 2.0) + (_180 + _174)) * 0.083333335816860198974609375) - _140) * (1.0 / _150), 0.0, 1.0);
        float _227 = _203 ? 0.0 : in.in_var_TEXCOORD5.x;
        float _229 = _200 ? 0.0 : in.in_var_TEXCOORD5.y;
        float2 _235;
        if (_203)
        {
          float2 _234 = _125;
          _234.x = in.in_var_TEXCOORD0.x + (_223 * 0.5);
          _235 = _234;
        }
        else
        {
          _235 = _125;
        }
        float2 _242;
        if (_200)
        {
          float2 _241 = _235;
          _241.y = _235.y + (_223 * 0.5);
          _242 = _241;
        }
        else
        {
          _242 = _235;
        }
        float _244 = _242.x - _227;
        float2 _245 = _67;
        _245.x = _244;
        float2 _248 = _245;
        _248.y = _242.y - _229;
        float _249 = _242.x + _227;
        float2 _250 = _67;
        _250.x = _249;
        float2 _252 = _250;
        _252.y = _242.y + _229;
        float _264 = fast::max(_216, _217) * 0.25;
        float _265 = ((!_218) ? (_205 + _140) : (_204 + _140)) * 0.5;
        float _267 = (((-2.0) * _226) + 3.0) * (_226 * _226);
        bool _268 = (_140 - _265) < 0.0;
        float _269 = texture0.sample(sampler0, _248, level(0.0)).y - _265;
        float _270 = texture0.sample(sampler0, _252, level(0.0)).y - _265;
        bool _275 = !(abs(_269) >= _264);
        float2 _281;
        if (_275)
        {
          float2 _280 = _248;
          _280.x = _244 - (_227 * 1.5);
          _281 = _280;
        }
        else
        {
          _281 = _248;
        }
        float2 _288;
        if (_275)
        {
          float2 _287 = _281;
          _287.y = _281.y - (_229 * 1.5);
          _288 = _287;
        }
        else
        {
          _288 = _281;
        }
        bool _289 = !(abs(_270) >= _264);
        float2 _296;
        if (_289)
        {
          float2 _295 = _252;
          _295.x = _249 + (_227 * 1.5);
          _296 = _295;
        }
        else
        {
          _296 = _252;
        }
        float2 _303;
        if (_289)
        {
          float2 _302 = _296;
          _302.y = _296.y + (_229 * 1.5);
          _303 = _302;
        }
        else
        {
          _303 = _296;
        }
        float2 _482;
        float2 _483;
        float _484;
        float _485;
        if (_275 || _289)
        {
          float _311;
          if (_275)
          {
            _311 = texture0.sample(sampler0, _288, level(0.0)).y;
          }
          else
          {
            _311 = _269;
          }
          float _317;
          if (_289)
          {
            _317 = texture0.sample(sampler0, _303, level(0.0)).y;
          }
          else
          {
            _317 = _270;
          }
          float _321;
          if (_275)
          {
            _321 = _311 - _265;
          }
          else
          {
            _321 = _311;
          }
          float _325;
          if (_289)
          {
            _325 = _317 - _265;
          }
          else
          {
            _325 = _317;
          }
          bool _330 = !(abs(_321) >= _264);
          float2 _337;
          if (_330)
          {
            float2 _336 = _288;
            _336.x = _288.x - (_227 * 2.0);
            _337 = _336;
          }
          else
          {
            _337 = _288;
          }
          float2 _344;
          if (_330)
          {
            float2 _343 = _337;
            _343.y = _337.y - (_229 * 2.0);
            _344 = _343;
          }
          else
          {
            _344 = _337;
          }
          bool _345 = !(abs(_325) >= _264);
          float2 _353;
          if (_345)
          {
            float2 _352 = _303;
            _352.x = _303.x + (_227 * 2.0);
            _353 = _352;
          }
          else
          {
            _353 = _303;
          }
          float2 _360;
          if (_345)
          {
            float2 _359 = _353;
            _359.y = _353.y + (_229 * 2.0);
            _360 = _359;
          }
          else
          {
            _360 = _353;
          }
          float2 _478;
          float2 _479;
          float _480;
          float _481;
          if (_330 || _345)
          {
            float _368;
            if (_330)
            {
              _368 = texture0.sample(sampler0, _344, level(0.0)).y;
            }
            else
            {
              _368 = _321;
            }
            float _374;
            if (_345)
            {
              _374 = texture0.sample(sampler0, _360, level(0.0)).y;
            }
            else
            {
              _374 = _325;
            }
            float _378;
            if (_330)
            {
              _378 = _368 - _265;
            }
            else
            {
              _378 = _368;
            }
            float _382;
            if (_345)
            {
              _382 = _374 - _265;
            }
            else
            {
              _382 = _374;
            }
            bool _387 = !(abs(_378) >= _264);
            float2 _394;
            if (_387)
            {
              float2 _393 = _344;
              _393.x = _344.x - (_227 * 4.0);
              _394 = _393;
            }
            else
            {
              _394 = _344;
            }
            float2 _401;
            if (_387)
            {
              float2 _400 = _394;
              _400.y = _394.y - (_229 * 4.0);
              _401 = _400;
            }
            else
            {
              _401 = _394;
            }
            bool _402 = !(abs(_382) >= _264);
            float2 _410;
            if (_402)
            {
              float2 _409 = _360;
              _409.x = _360.x + (_227 * 4.0);
              _410 = _409;
            }
            else
            {
              _410 = _360;
            }
            float2 _417;
            if (_402)
            {
              float2 _416 = _410;
              _416.y = _410.y + (_229 * 4.0);
              _417 = _416;
            }
            else
            {
              _417 = _410;
            }
            float2 _474;
            float2 _475;
            float _476;
            float _477;
            if (_387 || _402)
            {
              float _425;
              if (_387)
              {
                _425 = texture0.sample(sampler0, _401, level(0.0)).y;
              }
              else
              {
                _425 = _378;
              }
              float _431;
              if (_402)
              {
                _431 = texture0.sample(sampler0, _417, level(0.0)).y;
              }
              else
              {
                _431 = _382;
              }
              float _435;
              if (_387)
              {
                _435 = _425 - _265;
              }
              else
              {
                _435 = _425;
              }
              float _439;
              if (_402)
              {
                _439 = _431 - _265;
              }
              else
              {
                _439 = _431;
              }
              bool _444 = !(abs(_435) >= _264);
              float2 _451;
              if (_444)
              {
                float2 _450 = _401;
                _450.x = _401.x - (_227 * 12.0);
                _451 = _450;
              }
              else
              {
                _451 = _401;
              }
              float2 _458;
              if (_444)
              {
                float2 _457 = _451;
                _457.y = _451.y - (_229 * 12.0);
                _458 = _457;
              }
              else
              {
                _458 = _451;
              }
              bool _459 = !(abs(_439) >= _264);
              float2 _466;
              if (_459)
              {
                float2 _465 = _417;
                _465.x = _417.x + (_227 * 12.0);
                _466 = _465;
              }
              else
              {
                _466 = _417;
              }
              float2 _473;
              if (_459)
              {
                float2 _472 = _466;
                _472.y = _466.y + (_229 * 12.0);
                _473 = _472;
              }
              else
              {
                _473 = _466;
              }
              _474 = _473;
              _475 = _458;
              _476 = _439;
              _477 = _435;
            }
            else
            {
              _474 = _417;
              _475 = _401;
              _476 = _382;
              _477 = _378;
            }
            _478 = _474;
            _479 = _475;
            _480 = _476;
            _481 = _477;
          }
          else
          {
            _478 = _360;
            _479 = _344;
            _480 = _325;
            _481 = _321;
          }
          _482 = _478;
          _483 = _479;
          _484 = _480;
          _485 = _481;
        }
        else
        {
          _482 = _303;
          _483 = _288;
          _484 = _270;
          _485 = _269;
        }
        float _494;
        if (_203)
        {
          _494 = in.in_var_TEXCOORD0.y - _483.y;
        }
        else
        {
          _494 = in.in_var_TEXCOORD0.x - _483.x;
        }
        float _499;
        if (_203)
        {
          _499 = _482.y - in.in_var_TEXCOORD0.y;
        }
        else
        {
          _499 = _482.x - in.in_var_TEXCOORD0.x;
        }
        float _514 = fast::max(((_494 < _499) ? ((_485 < 0.0) != _268) : ((_484 < 0.0) != _268)) ? ((fast::min(_494, _499) * ((-1.0) / (_499 + _494))) + 0.5) : 0.0, (_267 * _267) * 0.75);
        float2 _520;
        if (_203)
        {
          float2 _519 = _125;
          _519.x = in.in_var_TEXCOORD0.x + (_514 * _223);
          _520 = _519;
        }
        else
        {
          _520 = _125;
        }
        float2 _527;
        if (_200)
        {
          float2 _526 = _520;
          _526.y = _520.y + (_514 * _223);
          _527 = _526;
        }
        else
        {
          _527 = _520;
        }
        _534 = float4(texture0.sample(sampler0, _527, level(0.0)).xyz, _66);
        break;
      }
    }
    _535 = _534;
  }
  out.out_var_SV_Target = float4(mix(float3(dot(_535.xyz, float3(0.2125000059604644775390625, 0.7153999805450439453125, 0.07209999859333038330078125))), _535.xyz, float3(in.in_var_TEXCOORD6)), 1.0);
  return out;
}

