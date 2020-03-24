#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct UDVSOutput
{
  float4 v_position [[position]];
  float2 uv;
};

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
main0(UDVSOutput in [[stage_in]],
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
