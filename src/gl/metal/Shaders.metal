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
      uchar4 color     [[attribute(2)]];
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
      out.color = float4(in.color) / float4(255.0);
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
