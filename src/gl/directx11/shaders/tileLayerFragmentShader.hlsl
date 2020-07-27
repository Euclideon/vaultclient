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
  float4 colour : COLOR0;
  float2 uv : TEXCOORD0;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

sampler colourSampler;
Texture2D colourTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);
  
  output.Color0 = float4(col.xyz * input.colour.xyz, col.w * input.colour.w);
  output.Normal = float4(0, 0, 0, 0);
  return output;
}