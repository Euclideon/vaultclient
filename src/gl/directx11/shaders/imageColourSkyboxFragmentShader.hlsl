cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 tintColour : COLOR0;
};

sampler sampler0;
Texture2D u_texture;

float4 main(PS_INPUT input) : SV_Target
{
  float4 colour = u_texture.Sample(sampler0, input.uv).rgba;
  float effectiveAlpha = min(colour.a, input.tintColour.a);
  return float4((colour.rgb * effectiveAlpha) + (input.tintColour.rgb * (1 - effectiveAlpha)), 1);
}
