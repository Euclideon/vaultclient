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
  float2 uv0  : TEXCOORD0;
  float2 uv1  : TEXCOORD1;
  float2 uv2  : TEXCOORD2;
};

sampler sampler0;
Texture2D texture0;

static float4 kernel[3] = { float4(0.0, 0.0, 0.0, 0.27901),
                            float4(1.0, 1.0, 1.0, 0.44198),
                            float4(0.0, 0.0, 0.0, 0.27901) };

float4 main(PS_INPUT input) : SV_Target
{
  float4 colour = float4(0.0, 0.0, 0.0, 0.0);

  colour += kernel[0] * texture0.Sample(sampler0, input.uv0);
  colour += kernel[1] * texture0.Sample(sampler0, input.uv1);
  colour += kernel[2] * texture0.Sample(sampler0, input.uv2);

  return colour;
}
