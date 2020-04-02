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
  float2 fLogDepth : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
  float Depth0 : SV_Depth;
};

sampler colourSampler;
Texture2D colourTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);

  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;

  output.Color0 = float4(col.xyz * input.colour.xyz, input.colour.w);
  return output;
}
