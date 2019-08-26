#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

// This should match CPU struct size
#define VERTEX_COUNT 2

// Compass Vertex Shader - g_CompassVertexShader
  struct CVSInput
  {
    float3 a_pos [[attribute(0)]];
    float3 a_normal [[attribute(1)]];
  };

  struct CVSOutput
  {
    float4 pos [[position]];
    float4 v_colour;
    float3 v_normal;
    float4 v_fragClipPosition;
    float3 v_sunDirection;
  };

  struct CVSUniforms
  {
    float4x4 u_viewProjectionMatrix;
    float4 u_colour;
    float3 u_sunDirection;
  };

  vertex CVSOutput
  compassVertexShader(CVSInput in [[stage_in]], constant CVSUniforms& uCVS [[buffer(1)]])
  {
    CVSOutput out;
    out.v_fragClipPosition = uCVS.u_viewProjectionMatrix * float4(in.a_pos, 1.0);
    out.pos = out.v_fragClipPosition;
    out.v_normal = (in.a_normal * 0.5) + 0.5;
    out.v_colour = uCVS.u_colour;
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
    float3 sheenColour = float3(1.0, 1.0, 0.9);
    return float4(in.v_colour.a * (ndotl * in.v_colour.rgb + edotr * sheenColour), 1.0);
  }

// Tile Vertex Shader - g_tileVertexShader
  struct TVSInput
  {
    float3 a_uv [[attribute(0)]];
  };

  struct TVSOutput
  {
    float4 finalClipPos [[position]];
    float4 v_color;
    float2 v_uv;
  };

  struct TVSUniforms
  {
    float4x4 u_projection;
    float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
    float4 u_color;
  };

  vertex TVSOutput
  tileVertexShader(TVSInput in [[stage_in]], constant TVSUniforms& uTVS [[buffer(1)]])
  {
    TVSOutput out;
    
    // TODO: could have precision issues on some devices
    out.finalClipPos = uTVS.u_projection * uTVS.u_eyePositions[int(in.a_uv.z)];
    
    out.v_uv = in.a_uv.xy;
    out.v_color = uTVS.u_color;
    return out;
  }

// Tile Fragment Shader - g_tileFragmentShader
  struct TFSInput
  {
    float4 v_position [[position]];
    float4 v_color;
    float2 v_uv;
  };

  fragment float4
  tileFragmentShader(TVSOutput in [[stage_in]], texture2d<float, access::sample> TFSimg [[texture(0)]], sampler TFSsampler [[sampler(0)]])
  {
    float4 col = TFSimg.sample(TFSsampler, in.v_uv);
    return float4(col.xyz * in.v_color.xyz, in.v_color.w);
  };

// Skybox Vertex Shader - g_vcSkyboxVertexShader
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
  skyboxVertexShader(SVSInput in [[stage_in]])
  {
    SVSOutput out;
    out.position = float4(in.a_position.xy, 0.0, 1.0);
    out.v_texCoord = float2(in.a_texCoord.x, 1.0 - in.a_texCoord.y);
    return out;
  }


// Skybox Fragment Shader Panorama - skyboxFragmentShaderPanarama
  struct SFSPUniforms
  {
    float4x4 u_inverseViewProjection;
  };

  fragment float4
  skyboxFragmentShaderPanarama(SVSOutput in [[stage_in]], constant SFSPUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
  {
    // work out 3D point
    float4 point3D = uSFS.u_inverseViewProjection * float4(in.v_texCoord * float2(2.0) - float2(1.0), 1.0, 1.0);
    point3D.xyz = normalize(point3D.xyz / point3D.w);
    float2 longlat = float2(atan2(point3D.x, point3D.y) + M_PI_F, acos(point3D.z));
    return SFSimg.sample(SFSsampler, longlat / float2(2.0 * M_PI_F, M_PI_F));
  }

// Skybox Fragment Shader TintColour - skyboxFragmentShaderImageColour
  struct SFSTCUniforms
  {
    float4 u_tintColour; //0 is full colour, 1 is full image
    float4 u_imageSize; //For purposes of tiling/stretching
  };

  fragment float4
  skyboxFragmentShaderImageColour(SVSOutput in [[stage_in]], constant SFSTCUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
  {
    float4 colour = SFSimg.sample(SFSsampler, in.v_texCoord / uSFS.u_imageSize.xy);
    float effectiveAlpha = min(colour.a, uSFS.u_tintColour.a);
    return float4((colour.rgb * effectiveAlpha) + (uSFS.u_tintColour.rgb * (1 - effectiveAlpha)), 1);
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
    float4 u_bottomColour;
    float4 u_topColour;

    float u_orientation;
    float u_width;
    float u_textureRepeatScale;
    float u_textureScrollSpeed;
    float u_time;
  };

  struct FVSEveryObject
  {
    float4x4 u_modelViewProjectionMatrix;
  };

  struct FVSInput
  {
    float3 a_position [[attribute(0)]];
    float2 a_uv [[attribute(1)]];
    float4 a_ribbonInfo [[attribute(2)]]; // xyz: expand floattor; z: pair id (0 or 1)
  };

  struct FVSOutput
  {
    float4 v_position [[position]];
    float4 v_colour;
    float2 v_uv;
  };

  vertex FVSOutput
  fenceVertexShader(FVSInput in [[stage_in]], constant FVSUniforms& uFVS [[buffer(1)]], constant FVSEveryObject& uFVSEO [[buffer(2)]])
  {
    FVSOutput out;

    // fence horizontal UV pos packed into Y channel
    out.v_uv = float2(mix(in.a_uv.y, in.a_uv.x, uFVS.u_orientation) * uFVS.u_textureRepeatScale - uFVS.u_time * uFVS.u_textureScrollSpeed, in.a_ribbonInfo.w);
    out.v_colour = mix(uFVS.u_bottomColour, uFVS.u_topColour, in.a_ribbonInfo.w);

    // fence or flat
    float3 worldPosition = in.a_position + mix(float3(0, 0, in.a_ribbonInfo.w) * uFVS.u_width, in.a_ribbonInfo.xyz, uFVS.u_orientation);

    out.v_position = uFVSEO.u_modelViewProjectionMatrix * float4(worldPosition, 1.0);
    return out;
  }


// Fence Fragment Shader - g_FenceFragmentShader
  struct FFSInput
  {
    float4 position [[position]];
    float4 v_colour;
    float2 v_uv;
  };

  fragment float4 fenceFragmentShader(FFSInput in [[stage_in]], texture2d<float, access::sample> FFSimg [[texture(0)]], sampler FFSsampler [[sampler(0)]])
  {
    float4 texCol = FFSimg.sample(FFSsampler, in.v_uv);
    return float4(texCol.xyz * in.v_colour.xyz, texCol.w * in.v_colour.w);
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
    float4 u_outlineColour;
    float4 u_outlineParams;
    float4 u_colourizeHeightColourMin;
    float4 u_colourizeHeightColourMax;
    float4 u_colourizeHeightParams;
    float4 u_colourizeDepthColour;
    float4 u_colourizeDepthParams;
    float4 u_contourColour;
    float4 u_contourParams;
  };

  struct UDFSOutput
  {
    float4 out_Colour [[color(0)]];
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
    
    float farPlane = uUDFS.u_screenParams.w;
    float nearPlane = uUDFS.u_screenParams.z;
    
    float4 fragWorldPosition = uUDFS.u_inverseViewProjection * float4(in.uv.x * 2.0 - 1.0, (1.0 - in.uv.y) * 2.0 - 1.0, depth, 1.0);
    fragWorldPosition = fragWorldPosition / fragWorldPosition.w;
    
    float2 worldColourMinMax = uUDFS.u_colourizeHeightParams.xy;
    float minMaxColourStrength = clamp((fragWorldPosition.z - worldColourMinMax.x) / (worldColourMinMax.y - worldColourMinMax.x), 0.0, 1.0);
    float3 minColour = mix(col.xyz, uUDFS.u_colourizeHeightColourMin.xyz, uUDFS.u_colourizeHeightColourMin.w);
    float3 maxColour = mix(col.xyz, uUDFS.u_colourizeHeightColourMax.xyz, uUDFS.u_colourizeHeightColourMax.w);
    col.xyz = mix(minColour, maxColour, minMaxColourStrength);

    float linearDepth = ((2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane))) * farPlane;
    float2 depthColourMinMax = uUDFS.u_colourizeDepthParams.xy;
    
    float depthColourStrength = clamp((linearDepth - depthColourMinMax.x) / (depthColourMinMax.y - depthColourMinMax.x), 0.0, 1.0);

    col.xyz = mix(col.xyz, uUDFS.u_colourizeDepthColour.xyz, depthColourStrength * uUDFS.u_colourizeDepthColour.w);
    
    float edgeOutlineWidth = uUDFS.u_outlineParams.x;
    if (edgeOutlineWidth > 0.0 && uUDFS.u_outlineColour.w > 0.0)
    {
      float3 sampleOffsets = float3(uUDFS.u_screenParams.xy, 0.0);
      float edgeOutlineThreshold = uUDFS.u_outlineParams.y;
      
      float d1 = UDFSdepthTexture.sample(UDFSdepthSampler, in.uv + sampleOffsets.xz);
      float d2 = UDFSdepthTexture.sample(UDFSdepthSampler, in.uv - sampleOffsets.xz);
      float d3 = UDFSdepthTexture.sample(UDFSdepthSampler, in.uv + sampleOffsets.zy);
      float d4 = UDFSdepthTexture.sample(UDFSdepthSampler, in.uv - sampleOffsets.zy);
      
      float wd0 = ((2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane))) * farPlane;
      float wd1 = ((2.0 * nearPlane) / (farPlane + nearPlane - d1 * (farPlane - nearPlane))) * farPlane;
      float wd2 = ((2.0 * nearPlane) / (farPlane + nearPlane - d2 * (farPlane - nearPlane))) * farPlane;
      float wd3 = ((2.0 * nearPlane) / (farPlane + nearPlane - d3 * (farPlane - nearPlane))) * farPlane;
      float wd4 = ((2.0 * nearPlane) / (farPlane + nearPlane - d4 * (farPlane - nearPlane))) * farPlane;
      
      float isEdge = 1.0 - step(wd0 - wd1, edgeOutlineThreshold) * step(wd0 - wd2, edgeOutlineThreshold) * step(wd0 - wd3, edgeOutlineThreshold) * step(wd0 - wd4, edgeOutlineThreshold);
      
      float3 edgeColour = mix(col.xyz, uUDFS.u_outlineColour.xyz, uUDFS.u_outlineColour.w);
      float minDepth = min(min(min(d1, d2), d3), d4);
      float4 edgeResult = float4(mix(col.xyz, edgeColour, isEdge), (depth + isEdge * (minDepth - depth)));

      col.xyz = edgeResult.xyz;
      depth = edgeResult.w; // to preserve outsides edges, depth written may be adjusted
    }
    
    float contourBandHeight = uUDFS.u_contourParams.y;
    
    float isCountour = step(contourBandHeight, fmod(fragWorldPosition.z, uUDFS.u_contourParams.x));
    float3 contourColour = mix(col.xyz, uUDFS.u_contourColour.xyz, uUDFS.u_contourColour.w);
    col.xyz = mix(contourColour, col.xyz, isCountour);
    
    out.out_Colour = float4(col.rgb, 1.0);// UD always opaque
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
    float3 colour;
};

struct WVSUniforms1
{
    float4 u_time;
};

struct WVSUniforms2
{
    float4 u_colourAndSize;
    float4x4 u_modelViewMatrix;
    float4x4 u_modelViewProjectionMatrix;
};

vertex WVSOutput
waterVertexShader(WVSInput in [[stage_in]], constant WVSUniforms1& uWVS1 [[buffer(1)]], constant WVSUniforms2& uWVS2 [[buffer(3)]])
{
    WVSOutput out;
    
    float uvScaleBodySize = uWVS2.u_colourAndSize.w; // packed here
    
    // scale the uvs with time
    float uvOffset = uWVS1.u_time.x * 0.0625;
    out.uv0 = uvScaleBodySize * in.pos.xy * float2(0.25, 0.25) - float2(uvOffset);
    out.uv1 = uvScaleBodySize * in.pos.yx * float2(0.50, 0.50) - float2(uvOffset, uvOffset * 0.75);
    
    out.fragEyePos = uWVS2.u_modelViewMatrix * float4(in.pos, 0.0, 1.0);
    out.colour = uWVS2.u_colourAndSize.xyz;
    out.pos = uWVS2.u_modelViewProjectionMatrix * float4(in.pos, 0.0, 1.0);
    
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
  float3 refractionColour = in.colour.xyz * mix(shallowFactor, deepFactor, distanceToShore);
  
  // reflection
  float4 worldFragPos = uWFS.u_inverseViewMatrix * float4(eyeReflectionDir, 0.0);
  float4 skybox = WFSSkybox.sample(WFSSBsampler, directionToLatLong(normalize(worldFragPos.xyz)));
  float3 reflectionColour = skybox.xyz;
  
  float3 finalColour = mix(reflectionColour, refractionColour, fresnel * 0.75) + float3(specular);
  return float4(finalColour, 1.0);
}


// g_PolygonP1N1UV1VertexShader
struct PNUVSInput
{
  float3 pos [[attribute(0)]];
  float2 uv [[attribute(1)]];
  float3 normal [[attribute(2)]];
};

struct PNUVSOutput
{
  float4 pos [[position]];
  float2 uv;
  float3 normal;
  float4 colour;
};

struct PNUVSUniforms1
{
  float4x4 u_viewProjectionMatrix;
};

struct PNUVSUniforms2
{
  float4x4 u_modelMatrix;
  float4 u_colour;
};

vertex PNUVSOutput
PNUVVertexShader(PNUVSInput in [[stage_in]], constant PNUVSUniforms1& PNUVS1 [[buffer(1)]], constant PNUVSUniforms2& PNUVS2 [[buffer(2)]])
{
  PNUVSOutput out;
  
  out.pos = PNUVS1.u_viewProjectionMatrix * (PNUVS2.u_modelMatrix * float4(in.pos, 1.0));
  out.uv = float2(in.uv.x, 1.0 - in.uv.y);
  out.normal = in.normal;
  out.colour = PNUVS2.u_colour;
  
  return out;
}


// g_PolygonP1N1UV1FragmentShader

fragment float4
PNUVFragmentShader(PNUVSOutput in [[stage_in]], texture2d<float, access::sample> PNUFSimg [[texture(0)]], sampler PNUFSsampler [[sampler(0)]])
{
    float4 col = PNUFSimg.sample(PNUFSsampler, in.uv);
    return float4(col.xyz, 1.0);
}


// g_PolygonP1UV1FragmentShader
struct PUVFSInput
{
    float4 pos [[position]];
    float2 uv;
    float4 colour;
};

fragment float4
PUVFragmentShader(PUVFSInput in [[stage_in]], texture2d<float, access::sample> PUFSimg [[texture(0)]], sampler PUFSsampler [[sampler(0)]])
{
    float4 col = PUFSimg.sample(PUFSsampler, in.uv);
    return col * in.colour;
}

// g_PolygonP1UV1VertexShader
struct PUVVSInput
{
    float3 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct PUVVSUniforms
{
    float4x4 u_modelViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize; // unused
};

vertex PUVFSInput
PUVVertexShader(PUVVSInput in [[stage_in]], constant PUVVSUniforms& uniforms [[buffer(1)]])
{
    PUVFSInput out;
    
    out.pos = uniforms.u_modelViewProjectionMatrix * float4(in.pos, 1.0);
    out.uv = float2(in.uv.x, 1.0 - in.uv.y);
    out.colour = uniforms.u_colour;
    
    return out;
}

// g_BillboardFragmentShader
struct BFSInput
{
    float4 pos [[position]];
    float2 uv;
    float4 colour;
};

fragment float4
billboardFragmentShader(BFSInput in [[stage_in]], texture2d<float, access::sample> BFSimg [[texture(0)]], sampler BFSsampler [[sampler(0)]])
{
    float4 col = BFSimg.sample(BFSsampler, in.uv);
    return col * in.colour;
}

// g_BillboardVertexShader
struct BVSInput
{
    float3 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct BVSUniforms
{
    float4x4 u_modelViewProjectionMatrix;
    float4 u_colour;
    float4 u_screenSize;
};

vertex BFSInput
billboardVertexShader(BVSInput in [[stage_in]], constant BVSUniforms& uniforms [[buffer(1)]])
{
    BFSInput out;
    
    out.pos = uniforms.u_modelViewProjectionMatrix * float4(in.pos, 1.0);
    out.pos.xy += uniforms.u_screenSize.z * out.pos.w * uniforms.u_screenSize.xy * float2(in.uv.x * 2.0 - 1.0, in.uv.y * 2.0 - 1.0); // expand billboard
    out.uv = float2(in.uv.x, 1.0 - in.uv.y);
    out.colour = uniforms.u_colour;
    
    return out;
}
