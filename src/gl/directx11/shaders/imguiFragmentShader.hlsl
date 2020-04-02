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
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
};

sampler TextureSampler;
Texture2D TextureTexture;

float4 main(PS_INPUT input) : SV_Target
{
  float4 out_col = input.col * TextureTexture.Sample(TextureSampler, input.uv);
  return out_col;
}
