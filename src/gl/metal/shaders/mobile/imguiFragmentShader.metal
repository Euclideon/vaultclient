#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct VertexOut {
  float4 position [[position]];
  float2 texCoords;
  float4 color;
};

fragment half4 main0(VertexOut in [[stage_in]], texture2d<half, access::sample> IMtexture [[texture(0)]], sampler imguiSampler [[sampler(0)]])
{
  half4 texColor = IMtexture.sample(imguiSampler, in.texCoords);
  return half4(in.color) * texColor;
}
