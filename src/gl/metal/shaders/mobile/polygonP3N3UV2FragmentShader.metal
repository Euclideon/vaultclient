#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct PNUVSOutput
{
  float4 pos [[position]];
  float2 uv;
  float3 normal;
  float4 color;
};

fragment float4
main0(PNUVSOutput in [[stage_in]], texture2d<float, access::sample> PNUFSimg [[texture(0)]], sampler PNUFSsampler [[sampler(0)]])
{
    float4 col = PNUFSimg.sample(PNUFSsampler, in.uv);
    float4 diffuseColour = col * in.color;

    // some fixed lighting
    float3 lightDirection = normalize(float3(0.85, 0.15, 0.5));
    float ndotl = dot(in.normal, lightDirection) * 0.5 + 0.5;
    float3 diffuse = diffuseColour.xyz * ndotl;

    return float4(diffuse, diffuseColour.a);
}
