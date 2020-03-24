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
  float2 fLogDepth : TEXCOORD1;
};

sampler sampler0;
Texture2D texture0;

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
  float Depth0 : SV_Depth;
};

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = texture0.Sample(sampler0, input.uv);
  output.Color0 = col * input.colour;

  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;

  return output;
}
