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
  float2 uv  : TEXCOORD0;
  float4 ribbonInfo : COLOR0; // xyz: expand vector; z: pair id (0 or 1)
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
};

cbuffer u_EveryFrame : register(b0)
{
  float4 u_bottomColour;
  float4 u_topColour;
  float4 u_worldUp; //can represent both vertical and horizontal orientations

  //           0: Positive expand
  // bits 0:3  1: Negative expand
  //           2: Uniform expand
  // bits 4:5  0: Use expand vector u_worldUp 
  //           1: Use expand vector ribbonInfo.xyz
  int u_options;
  float u_width;
  float u_textureRepeatScale;
  float u_textureScrollSpeed;
  float u_time;

  float _padding1;
  float _padding2;
  float _padding3;
};

cbuffer u_EveryObject : register(b1)
{
  float4x4 u_worldViewProjectionMatrix;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  output.colour = lerp(u_bottomColour, u_topColour, input.ribbonInfo.w);

  float3 expandVector;

  int fenceType = (u_options >> 2) & 1;
  output.uv.y = input.ribbonInfo.w;

  //input.uv.y: uv coord at the location of this vertex
  //input.uv.x: uv coord at the location of the expand vector
  if (fenceType == 0)
  {
    output.uv.x = input.uv.y * u_textureRepeatScale - u_time;
    expandVector = float3(u_worldUp.x, u_worldUp.y, u_worldUp.z);
  }
  else
  {
    output.uv.x = input.uv.x * u_textureRepeatScale - u_time;
    expandVector = float3(input.ribbonInfo.x, input.ribbonInfo.y, input.ribbonInfo.z);
  }

  float3 worldPosition;
  int pivot = (u_options & 0xF);
  if (pivot == 0) //positive expand
    worldPosition = input.pos + expandVector * u_width * input.ribbonInfo.w;
  else if (pivot == 1) //negative expand
    worldPosition = input.pos - expandVector * u_width * input.ribbonInfo.w;
  else // uniform expand
    worldPosition = input.pos + expandVector * u_width * (input.ribbonInfo.w - 0.5);

  output.pos = mul(u_worldViewProjectionMatrix, float4(worldPosition, 1.0));
  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
