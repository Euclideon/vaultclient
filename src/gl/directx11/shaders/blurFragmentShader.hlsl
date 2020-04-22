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

sampler colourSampler;
Texture2D colourTexture;

// keep the edges in tact in the red channel, and the blurred edges in the green
static float4 kernel[3] = { float4(0.0, 0.27901, 0.0, 0.0),
                            float4(1.0, 0.44198, 0.0, 0.0),
                            float4(0.0, 0.27901, 0.0, 0.0) };

float4 main(PS_INPUT input) : SV_Target
{
  float4 colour = float4(0.0, 0.0, 0.0, 0.0);

  colour += kernel[0] * colourTexture.Sample(colourSampler, input.uv0);
  colour += kernel[1] * colourTexture.Sample(colourSampler, input.uv1);
  colour += kernel[2] * colourTexture.Sample(colourSampler, input.uv2);

  return colour;
}
