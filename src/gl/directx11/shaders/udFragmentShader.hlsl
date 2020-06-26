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
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

sampler sceneColourSampler;
Texture2D sceneColourTexture;

sampler sceneDepthSampler;
Texture2D sceneDepthTexture;

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  float4 col = sceneColourTexture.Sample(sceneColourSampler, input.uv);
  float depth = sceneDepthTexture.Sample(sceneDepthSampler, input.uv).x;

  output.Color0 = float4(col.zyx, 1.0);// UD always opaque, UD is BGRA but uploaded as RGBA
  output.Depth0 = depth;

  output.Normal = packNormal(float3(0.0, 0.0, 0.0), 0.0, output.Depth0);
  return output;
}
