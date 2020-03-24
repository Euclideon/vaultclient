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

struct SFSPUniforms
{
  float4x4 u_inverseViewProjection;
};

fragment float4
main0(SVSOutput in [[stage_in]], constant SFSPUniforms& uSFS [[buffer(1)]], texture2d<float, access::sample> SFSimg [[texture(0)]], sampler SFSsampler [[sampler(0)]])
{
  // work out 3D point
  float4 point3D = uSFS.u_inverseViewProjection * float4(in.v_texCoord * float2(2.0) - float2(1.0), 1.0, 1.0);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  float2 longlat = float2(atan2(point3D.x, point3D.y) + M_PI_F, acos(point3D.z));
  return SFSimg.sample(SFSsampler, longlat / float2(2.0 * M_PI_F, M_PI_F));
}
