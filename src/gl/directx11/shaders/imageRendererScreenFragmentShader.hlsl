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
  float4 colour : COLOR0;
};

sampler albedoSampler;
Texture2D albedoTexture;

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
};

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = albedoTexture.Sample(albedoSampler, input.uv);
  output.Color0 = col * input.colour;
  return output;
}
