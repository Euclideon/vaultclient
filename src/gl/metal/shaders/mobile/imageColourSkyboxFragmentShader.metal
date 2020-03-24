#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct SVSOutput
{
  float4 position [[position]];
  float2 v_texCoord;
};

struct SFSTCUniforms
{
  float4 u_tintColor; //0 is full color, 1 is full image
  float4 u_imageSize; //For purposes of tiling/stretching
};

fragment float4
main0(SVSOutput in [[stage_in]], constant SFSTCUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
{
  float4 color = SFSimg.sample(SFSsampler, in.v_texCoord / uSFS.u_imageSize.xy);
  float effectiveAlpha = min(color.a, uSFS.u_tintColor.a);
  return float4((color.rgb * effectiveAlpha) + (uSFS.u_tintColor.rgb * (1 - effectiveAlpha)), 1);
}
