#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

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

vertex VertexOut main0(VertexIn in [[stage_in]], constant Uniforms& uIMGUI [[buffer(1)]])
{
  VertexOut out;
  out.position = uIMGUI.projectionMatrix * float4(in.position, 0, 1);
  out.texCoords = in.texCoords;
  out.color = in.color;
  return out;
}
