#include <metal_stdlib>
#include <metal_matrix>
#include <metal_uniform>
#include <metal_texture>
using namespace metal;

struct CVSOutput
{
  float4 pos [[position]];
  float4 v_color;
  float3 v_normal;
  float4 v_fragClipPosition;
  float3 v_sunDirection;
};

fragment float4
main0(CVSOutput in [[stage_in]])
{
  float3 fakeEyeVector = normalize(in.v_fragClipPosition.xyz / in.v_fragClipPosition.w);
  float3 worldNormal = in.v_normal * float3(2.0) - float3(1.0);
  float ndotl = 0.5 + 0.5 * (-dot(in.v_sunDirection, worldNormal));
  float edotr = max(0.0, -dot(-fakeEyeVector, worldNormal));
  edotr = pow(edotr, 60.0);
  float3 sheenColor = float3(1.0, 1.0, 0.9);
  return float4(in.v_color.a * (ndotl * in.v_color.rgb + edotr * sheenColor), 1.0);
}
