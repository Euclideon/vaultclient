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
main0(UDVSOutput in [[stage_in]], constant UDSUniforms& uniforms [[buffer(1)]], texture2d<float, access::sample> UDSimg [[texture(0)]], sampler UDSSampler [[sampler(0)]], depth2d<float, access::sample> UDSimg2 [[texture(1)]], sampler UDSSampler2 [[sampler(1)]])
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
