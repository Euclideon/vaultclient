cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

//Input Format
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
};

//Output Format
struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

sampler colourSampler;
Texture2D colourTexture;

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 texCol = colourTexture.Sample(colourSampler, input.uv);
  output.Color0 = float4(texCol.xyz * input.colour.xyz, texCol.w * input.colour.w);

  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;

  output.Normal = packNormal(float3(0.0, 0.0, 1.0), 0.0, output.Depth0);
  return output;
}
