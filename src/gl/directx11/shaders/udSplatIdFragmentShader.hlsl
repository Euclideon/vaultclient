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
};

cbuffer u_params : register(b0)
{
  float4 u_idOverride;
};

sampler sceneColourSampler;
Texture2D sceneColourTexture;

bool floatEquals(float a, float b)
{
  return abs(a - b) <= 0.0015f;
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = sceneColourTexture.Sample(sceneColourSampler, input.uv);

  output.Color0 = float4(0.0, 0.0, 0.0, 0.0);
  if ((u_idOverride.w == 0.0 || floatEquals(u_idOverride.w, col.w)))
  {
    output.Color0 = float4(col.w, 0, 0, 1.0);
  }

  return output;
}
