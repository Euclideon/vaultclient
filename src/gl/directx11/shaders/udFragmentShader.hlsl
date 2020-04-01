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
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
  float Depth0 : SV_Depth;
};

sampler sampler0;
Texture2D texture0;

sampler sampler1;
Texture2D texture1;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 col = texture0.Sample(sampler0, input.uv);
  float depth = texture1.Sample(sampler1, input.uv).x;

  output.Color0 = float4(col.zyx, 1.0);// UD always opaque, UD is BGRA but uploaded as RGBA
  output.Depth0 = depth;

  return output;
}
