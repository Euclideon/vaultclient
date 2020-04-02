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
  float2 uv  : TEXCOORD0;
  float3 normal : NORMAL;
  float4 colour : COLOR0;
  float2 fLogDepth : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  output.Color0 = input.colour;

  return output;
}
