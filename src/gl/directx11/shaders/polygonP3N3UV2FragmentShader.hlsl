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
  float3 normal : NORMAL;
  float4 colour : COLOR0;
  float2 fLogDepth : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

sampler albedoSampler;
Texture2D albedoTexture;

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = albedoTexture.Sample(albedoSampler, input.uv);
  float4 diffuseColour = col * input.colour;

  output.Color0 = diffuseColour;

  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;

  // Some Polygon models have normals, some do not - disable for now
  output.Normal = packNormal(input.normal, input.objectInfo.x, output.Depth0); 
  
  // DISABLED FOR opaque geometry
  // conditionally disable selection (using alpha-blend)
  //output.Normal.w = input.objectInfo.y;
  
  return output;
}
