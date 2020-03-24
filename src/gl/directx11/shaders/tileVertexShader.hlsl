cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct VS_INPUT
{
  float3 pos : POSITION;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 uv : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
};

// This should match CPU struct size
#define VERTEX_COUNT 3

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_projection;
  float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
  float4 u_colour;
  float4 u_uvOffsetScale;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  // note: could have precision issues on some devices
  float4 finalClipPos = mul(u_projection, u_eyePositions[int(input.pos.z)]);
  output.colour = u_colour;
  output.uv = u_uvOffsetScale.xy + u_uvOffsetScale.zw * input.pos.xy;
  output.pos = finalClipPos;

  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
