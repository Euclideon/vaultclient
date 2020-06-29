cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct VS_INPUT
{
  float2 pos : POSITION;
  float2 uv  : TEXCOORD0;
  float4 col : COLOR0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_worldViewProjectionMatrix;
  float4 u_screenSize; // objectId.w
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = mul(u_worldViewProjectionMatrix, float4(0.0, 0.0, 0.0, 1.0));
  output.pos.xy += u_screenSize.xy * input.pos.xy * output.pos.w; // expand

  output.col = input.col;
  output.uv = input.uv;
  output.fLogDepth.x = 1.0 + output.pos.w;
  output.objectInfo = u_screenSize.w;
  
  return output;
}
