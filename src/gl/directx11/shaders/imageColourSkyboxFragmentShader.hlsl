cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 tintColour : COLOR0;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};
  
sampler albedoSampler;
Texture2D albedoTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 colour = albedoTexture.Sample(albedoSampler, input.uv).rgba;
  float effectiveAlpha = min(colour.a, input.tintColour.a);
  output.Color0 = float4((colour.rgb * effectiveAlpha) + (input.tintColour.rgb * (1 - effectiveAlpha)), 1.0);
  
  output.Normal = float4(0,0,0,0);
  return output;
}
