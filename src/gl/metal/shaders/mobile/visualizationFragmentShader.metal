#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct VVSOutput
{
  float4 v_position [[position]];
  float2 uv;
};

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
main0(VVSOutput in [[stage_in]],
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
