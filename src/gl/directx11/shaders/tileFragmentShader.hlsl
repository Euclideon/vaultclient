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
  float2 depth : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
};

sampler colourSampler;
Texture2D colourTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);

  output.Color0 = float4(col.xyz * input.colour.xyz, input.colour.w);
  
  float scale = 1.0 / (u_clipZFar - u_clipZNear);
  float bias = -(u_clipZNear * 0.5);
  output.Color0.a = (input.depth.x / input.depth.y) * scale + bias; // depth packed here
  return output;
}
